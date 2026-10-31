#include "fault.h"

#include <stdio.h>

static const char *const messages[EXIT_COUNT] = {
    [EXIT_OK]         = "Успешное завершение программы.",
    [EXIT_ERROR_FILE] = "Ошибка файла, данных, памяти.",
    [EXIT_ERROR_ARG]  = "Ошибка аргументов.",
};

int status(int exit_code, const char *msg) {
      if (exit_code < 0 || exit_code >= EXIT_COUNT) {
        fprintf(stderr, "Внутренняя ошибка: неизвестный код завершения %d.\n", exit_code);
        return EXIT_ERROR_FILE;
    }

    const char *text = messages[exit_code];
    if (msg == NULL || *msg == '\0') {
        fprintf(stderr, "%s\n", text);
    } else {
        fprintf(stderr, "%s %s\n", text, msg);
    }

    return exit_code;
}