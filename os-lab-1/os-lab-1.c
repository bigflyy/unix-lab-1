#include <stdio.h> 
#include <unistd.h> 
#include <pthread.h> 

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;    
int event_ready = 0; 

int shared_data = 0;


void* provider_thread(void* arg) {
    int counter = 0;
    while(1) { 
        sleep(1); 

        pthread_mutex_lock(&lock);

        while (event_ready) {
            pthread_cond_wait(&cond1, &lock); 
        }

        counter++;
        shared_data = counter; 
        event_ready = 1; 

        printf("Поставщик: инициировано событие %d (данные: %d)\n", counter, shared_data);

        pthread_cond_signal(&cond1); 
        pthread_mutex_unlock(&lock);
    }
}

void* consumer_thread(void* arg) {
    while(1) {
        pthread_mutex_lock(&lock);

        while (!event_ready) {
            pthread_cond_wait(&cond1, &lock); 
        }

        int current_value = shared_data; 
        event_ready = 0; 

        printf("Потребитель: получено событие (данные: %d)\n", current_value);

        pthread_cond_signal(&cond1); 
        pthread_mutex_unlock(&lock);
    }
}

int main() {
    pthread_t provider, consumer;

    // Создаем потоки
    pthread_create(&provider, NULL, provider_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);

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