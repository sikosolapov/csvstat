#ifndef FAULT_H
#define FAULT_H

/* 
 * Возвращает 0 ~ успешное завершение программы.
 * Возвращает 1 ~ ошибка файла, данных, памяти.
 * Возвращает 2 ~ ошибка аргументов.
 */
typedef enum {
    EXIT_OK = 0,
    EXIT_ERROR_FILE = 1,
    EXIT_ERROR_ARG = 2,
    EXIT_COUNT 
} ExitCode;

/*
 * Функция выводит сообщение об ошибке для заданного кода завершения.
 * Допустимые значения exit_code: от 0 до (EXIT_COUNT - 1).
 * Пустая строка ('\0') обрабатывается так же, как NULL.
 * При exit_code == EXIT_OK сообщения не выводятся (дополнительный msg игнорируется).
 */
int status(int exit_code, const char *msg);

#endif /* FAULT_H */