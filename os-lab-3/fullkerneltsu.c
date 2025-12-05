// tsulab.c

// Подключение заголовков, как указано в материалах (slide 4)
#include <linux/kernel.h>   // Для функций ядра, таких как pr_info
#include <linux/module.h>   // Необходим для всех модулей (slide 4)

// Функция, вызываемая при загрузке модуля (slide 2 & 3)
// Используем pr_info для вывода сообщения в буфер лога ядра (slide 2 & 3)
int init_module(void) {
    pr_info("Welcome to the Tomsk State University\n"); // Сообщение при загрузке
    return 0; // Успешная инициализация
}

// Функция, вызываемая при выгрузке модуля (slide 2 & 3)
void cleanup_module(void) {
    pr_info("Tomsk State University forever!\n"); // Сообщение при выгрузке
}

// Указание лицензии модуля (slide 2 & 3)
// Необходимо для корректной работы модуля
MODULE_LICENSE("GPL");