#include "fault.h"
#include <stdio.h>

static const char *const messages[EXIT_COUNT] = {
    [EXIT_OK]         = "", // Все норм, поэтому ничего не надо выводить. В выводе будут данные.
    [EXIT_ERROR_FILE] = "Ошибка файла, данных, памяти.",
    [EXIT_ERROR_ARG]  = "Ошибка аргументов.",
};

int status(int exit_code, const char *message) {
      if (exit_code < 0 || exit_code >= EXIT_COUNT) {
        fprintf(stderr, "Внутренняя ошибка: неизвестный код завершения %d.\n", exit_code);
        return EXIT_ERROR_FILE;
    }

    if (exit_code == EXIT_OK) {
        return EXIT_OK;
    }

    const char *text = messages[exit_code];

    if (message == NULL || *message == '\0') {
        fprintf(stderr, "%s\n", text);
    } else {
        fprintf(stderr, "%s %s\n", text, message);
    }
    return exit_code;
}