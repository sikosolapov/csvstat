#include "fault.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return status(EXIT_ERROR_ARG, "Недостаточно аргументов.");
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        return status(EXIT_ERROR_FILE, "Не удалось открыть файл.");
    }

    // Здесь можно добавить обработку файла

    fclose(file);
    return EXIT_OK;
}