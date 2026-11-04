#ifndef CSV_READER_H
#define CSV_READER_H

#include <stddef.h>

/*
 * Типы статусов для чтения CSV-файлов:
 * CSV_OK - успешное чтение поля CSV.
 * CSV_READ_END - достигнут конец строки CSV. Показывает, что нужно перейти к следующей строке CSV.
 * CSV_EOF - достигнут конец файла (data != NULL — последнее поле успешно прочитано, но завершающего '\n' нет; data == NULL — конец файла, больше полей нет).
 * CSV_ERROR_FILE - произошла ошибка при чтении файла/ выделения памяти.
 * CSV_ERROR_FORMAT - произошла ошибка формата CSV (например, несоответствие кавычек).
 */
typedef enum {
    CSV_OK,
    CSV_READ_END,
    CSV_EOF,
    CSV_ERROR_FILE,
    CSV_ERROR_FORMAT
} CSVstatus;
typedef enum {false, true} bool;

/*
 * Структура для хранения результата чтения поля CSV:
 * data - указатель на строку с данными поля CSV.
 * length - длина строки данных поля CSV.
 * has_quotes - Флаг, указывающий кавычки, окружающие поле CSV. (0 - нет кавычек, 1 - есть кавычки)
 * status - статус чтения CSV (CSV_OK, CSV_READ_END, CSV_EOF, CSV_ERROR_FILE, CSV_ERROR_FORMAT).
 */
typedef struct {
    char *data; 
    size_t length;        
    bool has_quotes;    
    CSVstatus status;   
} CSVfieldresult;

typedef struct{
    char delimiter;
    bool multiline;
} CSVoptions;

typedef struct CSVReader CSVReader;

/* Функции для чтения CSV-файлов:
 * Создает новый объект CSVReader для чтения CSV-файла.
 * Читает следующее поле CSV из объекта CSVReader.
 * Сбрасывает состояние объекта CSVReader -> Начинаем чтение с начала. Для повторного чтения CSV-файла для stats.
 * Полное освобождение ресурсов и закрытие файла, связанных с объектом CSVReader.
 * Освобождает ресурсы для конкретного результата чтения поля CSV.
 */
CSVReader *csv_reader_create(const char *filename, CSVoptions options);
CSVfieldresult csv_reader_read_field(CSVReader *reader);
#define CSV_RESET_OK 0
#define CSV_RESET_ERR -1
int csv_reader_reset(CSVReader *reader);
void csv_reader_destroy(CSVReader *reader);
void csv_field_free(CSVfieldresult *result);

#endif /* CSV_READER_H */