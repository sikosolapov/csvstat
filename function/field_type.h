#ifndef FIELD_TYPE_H
#define FIELD_TYPE_H
#include "csv_reader.h"


typedef enum {
    FIELD_TYPE_NULL,
    FIELD_TYPE_INT,
    FIELD_TYPE_DOUBLE,
    FIELD_TYPE_STRING
} FieldType;

/*
* Функция для определения типа
* s — содержимое поля без внешних CSV-кавычек.
* has_quotes — было ли поле заключено в CSV-кавычки.
* scientific — разрешена ли научная запись чисел.
* Пробелы и табуляции по краям обрабатываются согласно заданию лабораторной.
* NULL и пустая строка обозначаются FIELD_TYPE_NULL.
* Числа проверяются по синтаксису и допустимому диапазону согласно спецификации.
*/
FieldType type_inference(const char *s, bool has_quotes, bool scientific);
size_t field_check_and_cut(char *s, bool has_quotes); // Проверка на пробелы и табуляцию с последующим их .

#endif /* FIELD_TYPE_H */