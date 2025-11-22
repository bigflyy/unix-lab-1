#include <arpa/inet.h>
#include <signal.h>
#include <iostream>

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
        std::cout <<"socket";
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(42424);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cout <<"bind";
        close(listenSocket);
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == -1) {
        std::cout <<"listen";
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

    int activeClient = -1;
    std::cout << "Server started on port 42424" << std::endl;

    while (true) {
        // Подготовка множества файловых дескрипторов
        fd_set fds;
        FD_ZERO(&fds); 
        FD_SET(listenSocket, &fds);
        int maxFd = listenSocket; 

        if (activeClient != -1) {  // Если есть активный клиент
            FD_SET(activeClient, &fds);
            if (activeClient > maxFd) maxFd = activeClient; 
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
            std::cout <<"pselect error";
            break;
        }

        // Проверка нового подключения
        if (FD_ISSET(listenSocket, &fds)) {
            int clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == -1) {
                std::cout <<"accept";
            } else {
                std::cout << "New connection: " << clientSocket << std::endl;
                // Закрываем предыдущее соединение
                if (activeClient != -1) {  // Если был активный клиент
                    close(activeClient);
                    std::cout << "Closed previous connection: " << activeClient << std::endl;
                }

                activeClient = clientSocket;  // Сохранить нового клиента
                std::cout << "Active connection: " << clientSocket << std::endl;
            }
        }

        // Проверка данных от клиента
        if (activeClient != -1 && FD_ISSET(activeClient, &fds)) {  
            char buffer[1024];
            ssize_t bytesRead = recv(activeClient, buffer, sizeof(buffer), 0);
            
            if (bytesRead > 0) {
                std::cout << "Received " << bytesRead 
                          << " bytes from connection " << activeClient << std::endl;
            } else {
                if (bytesRead == 0) {
                    std::cout << "Connection closed by client: " << activeClient << std::endl;
                } else {
                    std::cout <<"recv";
                }
                close(activeClient);
                activeClient = -1;  // Обнулить переменную
            }
        }
    }
    // Завершение работы
    if (activeClient != -1) {  // Если есть активный клиент
        close(activeClient);
    }
    close(listenSocket);
    return 0;
}