#include <stdio.h> // printf input output
#include <unistd.h> // sleep
#include <pthread.h> // с потоками 


// Глобальные переменные для синхронизации и состояния
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;    
int event_ready = 0; // Флаг, сигнализирующий о наступлении события

// Указатель, представляющий "несериализуемые данные"
// Для простоты, он будет указывать на глобальную переменную.
int shared_data_value = 0;

// Функция, выполняемая потоком-поставщиком
void* provider_thread(void* arg) {
    int counter = 0;
    while(1) { // В реальном коде обычно добавляется условие остановки
        sleep(1); // Задержка в 1 секунду

        pthread_mutex_lock(&lock);

        // Проверяем, если событие еще не обработано, ждем
        // (Хотя в этой схеме с одним consumer это маловероятно)
        while (event_ready) {
            // Если event_ready == 1, мы ждем, пока consumer его сбросит.
            // Это предотвращает "гонку" между provider и consumer.
            // В данном случае, т.к. consumer всегда сбрасывает после обработки,
            // и provider ждет 1 секунду, это условие вряд ли сработает,
            // но оно важно для корректности в других сценариях.
            pthread_cond_wait(&cond1, &lock); // Ждем, освобождая мьютекс
        }

        // Производим "событие"
        counter++;
        shared_data_value = counter; // "Данные" для передачи
        event_ready = 1; // Устанавливаем флаг

        printf("Поставщик: инициировано событие %d (данные: %d)\n", counter, shared_data_value);

        pthread_cond_signal(&cond1); // Уведомляем один ожидающий поток (потребитель)
        pthread_mutex_unlock(&lock);
    }
}

// Функция, выполняемая потоком-потребителем
void* consumer_thread(void* arg) {
    while(1) { // В реальном коде обычно добавляется условие остановки
        pthread_mutex_lock(&lock);

        // Ждем, пока событие не будет готово
        while (!event_ready) {
            pthread_cond_wait(&cond1, &lock); // Ждем, освобождая мьютекс
        }

        // Событие готово, обрабатываем его
        int current_value = shared_data_value; // Получаем "данные"
        event_ready = 0; // Сбрасываем флаг, сигнализируя, что событие обработано

        printf("Потребитель: получено событие (данные: %d)\n", current_value);

        pthread_cond_signal(&cond1); // Уведомляем provider, что можно продолжать
        pthread_mutex_unlock(&lock);
    }
}

int main() {
    pthread_t provider, consumer;

    // Создаем потоки
    if (pthread_create(&provider, NULL, provider_thread, NULL) != 0) {
        printf("Ошибка при создании потока-поставщика");
        return 1;
    }
    if (pthread_create(&consumer, NULL, consumer_thread, NULL) != 0) {
        printf("Ошибка при создании потока-потребителя");
        return 1;
    }

    // завершим потоки через 10 секунд
    sleep(10);

    // Прерывание потоков 
    pthread_cancel(provider);
    pthread_cancel(consumer);

    // Главный поток ждет завершения других потоков
    pthread_join(provider, NULL);
    pthread_join(consumer, NULL);

    // Уничтожаем мьютекс и условную переменную
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);

    return 0;
}