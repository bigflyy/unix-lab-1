#include <arpa/inet.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <set>

// глобальный флаг 
volatile sig_atomic_t wasSigHup = 0;

// установить флаг = 1 (функция обработки сигнала sighup)
void sigHupHandler(int) {
    wasSigHup = 1;
}

int main() {
    // Создание и настройка сокета
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(42424);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listenSocket);
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == -1) {
        perror("listen");
        close(listenSocket);
        return 1;
    }

    // Настройка обработчика сигнала
    struct sigaction sa;
    sigaction(SIGHUP, nullptr, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;         
    sigaction(SIGHUP, &sa, nullptr);

    // Блокировка сигнала
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    std::set<int> clients;
    std::cout << "Server started on port 42424" << std::endl;

    while (true) {
        // Подготовка множества файловых дескрипторов
        fd_set fds;
        FD_ZERO(&fds); 
        FD_SET(listenSocket, &fds);
        int maxFd = listenSocket; 

        for (int client : clients) {
            FD_SET(client, &fds);
            if (client > maxFd) maxFd = client; 
        }

        // Ожидание событий
        int ready = pselect(maxFd + 1, &fds, nullptr, nullptr, nullptr, &origMask);
        if (ready == -1) {
            // был прерван сигналом
            if (errno == EINTR) {
                if (wasSigHup) {
                    std::cout << "Received SIGHUP signal" << std::endl;
                    wasSigHup = 0;
                }
                continue;
            }
            perror("pselect");
            break;
        }

        // Проверка нового подключения
        if (FD_ISSET(listenSocket, &fds)) {
            int clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == -1) {
                perror("accept");
            } else {
                std::cout << "New connection: " << clientSocket << std::endl;
                // Закрываем предыдущие соединения (оставляем только одно)
                for (int client : clients) {
                    close(client);
                    std::cout << "Closed previous connection: " << client << std::endl;
                }
                clients.clear();
                
                clients.insert(clientSocket);
                std::cout << "Active connection: " << clientSocket << std::endl;
            }
        }

        // Проверка данных от клиентов
        std::vector<int> toRemove;
        for (int client : clients) {
            if (FD_ISSET(client, &fds)) {
                char buffer[1024];
                ssize_t bytesRead = recv(client, buffer, sizeof(buffer), 0);
                
                if (bytesRead > 0) {
                    std::cout << "Received " << bytesRead 
                              << " bytes from connection " << client << std::endl;
                } else {
                    if (bytesRead == 0) {
                        std::cout << "Connection closed by client: " << client << std::endl;
                    } else {
                        perror("recv");
                    }
                    close(client);
                    toRemove.push_back(client);
                }
            }
        }

        // Удаление закрытых соединений
        for (int client : toRemove) {
            clients.erase(client);
        }
    }
    // Завершение работы
    for (int client : clients) {
        close(client);
    }
    close(listenSocket);
    return 0;
}