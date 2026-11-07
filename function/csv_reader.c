#include "csv_reader.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/*
* Структура CSVReader для чтения CSV-файлов.
* file - указатель на открытый файл CSV.
* options - структура CSVoptions, содержащая параметры чтения CSV.
* in_record - флаг, указывающий, находится ли читатель в начале новой записи CSV.
* probably_newline - счетчик, используемый для определения количества пустых строк внутри записи CSV.
*/

struct CSVReader {
    FILE *file;
    CSVoptions options;
    bool in_record;
    size_t probably_newline; 
};
// Проверка разделителя
bool csv_delimiter_check (char d){
    unsigned char D = (unsigned char)d;
    return D != 0 && D < 128 && d != ' ' && d != '"' && d != '\t' && d != '\r' && d != '\n';
}
// Создает новый объект CSVReader для чтения CSV-файла.
CSVReader *csv_reader_create(const char *filename, CSVoptions options){
    if (!filename || !csv_delimiter_check(options.delimiter)){
        return NULL;
    }
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    CSVReader *reader = malloc(sizeof(CSVReader));
    if (!reader) {
        fclose(file);
        return NULL;
    }
    reader->file = file;
    reader->options = options;
    reader->in_record = true;
    reader->probably_newline = 0;
    return reader;
}

//Полное освобождение ресурсов и закрытие файла, связанных с объектом CSVReader.
void csv_reader_destroy(CSVReader *reader){
    if (reader) {
        if (reader->file){
            fclose(reader->file);
        }
        free(reader);
    }
}

// Освобождает ресурсы для конкретного результата чтения поля CSV. Можно вызвать в любом случае, даже если чтение поля CSV завершилось с ошибкой.
void csv_field_free(CSVfieldresult *result){
    if (!result) return;
    free(result->data);
    result->data = NULL;
    result->length = 0;
    result->has_quotes = false;
}

// Сбрасывает состояние объекта CSVReader -> Начинаем чтение с начала. Для повторного чтения CSV-файла для stats.
int csv_reader_reset(CSVReader *reader){
    assert(reader != NULL);
    if (fseek(reader->file, 0, SEEK_SET) != 0) {
        return CSV_RESET_ERR;
    }
    clearerr(reader->file);
    reader->in_record = true;
    reader->probably_newline = 0;
    return CSV_RESET_OK;
}
// Функции, чтобы не ctrl-c, ctrl-v 
static CSVfieldresult result_out(CSVstatus st){
    CSVfieldresult result = {NULL, 0, false, st};
    return result;
}
static bool domain_expansion(char **buffer, size_t *size){
    char *tmp = realloc(*buffer, *size * 2);
    if (!tmp){
        free(*buffer);
        *buffer = NULL;
        return false;
    }
    *buffer = tmp;
    *size *= 2;
    return true;
}
/*
* Возвращает пустой результат чтения поля CSV с данными, указывающими на пустую строку.
* Нужен для проверки есть ли пустые строки внутри записи CSV, используется probably_newline.
*/
static CSVfieldresult empty_field_result(void) {
    return result_out(CSV_READ_END);
}
// Читает следующее поле CSV из объекта CSVReader.
CSVfieldresult csv_reader_read_field(CSVReader *reader){
    assert(reader != NULL);
    assert(reader->file != NULL);
    /*
    * Если мы находимся внутри записи CSV, проверяем, есть ли ожидаемые пустые строки.
    */
    if (reader->in_record){
        if (reader->probably_newline > 0){
            reader->probably_newline--;
            return empty_field_result();
        }
        int first_char = fgetc(reader->file);
        if (first_char == EOF){
            CSVfieldresult result = {NULL, 0, false,  ferror(reader->file) ? CSV_ERROR_FILE : CSV_EOF};
            return result;
        }
        if (first_char == '\n'){
            size_t count = 1;
            int next;
            while ((next = fgetc(reader->file)) == '\n'){
                count++;
            }
            if (ferror(reader->file)){
                CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
                return result;
            }
            if (next == EOF){
                CSVfieldresult result = {NULL, 0, false, CSV_EOF};
                return result;
            }
            if (ungetc(next, reader->file) == EOF){
                CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
                return result;
            }
            reader->probably_newline = count - 1;
            return empty_field_result();
        }
        if (ungetc(first_char, reader->file) == EOF){
            CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
            return result;
        }
    }
    /*
    * Основное чтение символов из файла CSV.
    */
    int c = fgetc(reader->file);
    size_t len = 0, size = 16;
    char *buffer = malloc(size);
    if (!buffer) {
        CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
        return result;
    }
    bool has_quotes = false;
    bool closed_quotes = false;
    CSVstatus status = CSV_OK;
    while (c == ' ' || c == '\t'){
        if (len + 1 >= size){
            if (!domain_expansion(&buffer, &size)){
                return result_out(CSV_ERROR_FILE);
            }
        }
        buffer[len] = (char)c;
        len++;
        c = fgetc(reader->file);
    }
    
    if (c == '"'){
        has_quotes = true;
        len = 0;
        c = fgetc(reader->file);
    }
    /*
    * Основной цикл чтения поля CSV.
    */
    while (c != EOF){ 
        if (has_quotes && !closed_quotes){
            if (c == '\n' && !reader->options.multiline){
                status = CSV_ERROR_FORMAT;
                break;
            }
            if (c == '"'){
                int next_c = fgetc(reader->file);
                if (next_c == '"'){
                    c = '"';
                } else {
                    closed_quotes = true;
                    c = next_c;
                    continue;
                }
            }
        } else if (has_quotes && closed_quotes) {
            if (c == ' ' || c == '\t') {
                c = fgetc(reader->file);
                continue;
            } 
            if (c == '\n'){
                status = CSV_READ_END;
                break;
            }
            if (c == (unsigned char)reader->options.delimiter){
                status = CSV_OK;
                break;
            }
            status = CSV_ERROR_FORMAT;
            break;
        } else{
            if (c == '"'){
                status = CSV_ERROR_FORMAT;
                break;
            }
            if (c == '\n'){
                status = CSV_READ_END;
                break;
            }
            if (c == (unsigned char)reader->options.delimiter){
                status = CSV_OK;
                break;
            }
        }
        /*
        *Если длина буфера достигла текущего размера, увеличиваем размер буфера вдвое, 
        *чтобы точно вместить все данные поля.
        */ 
        if (len + 1 >= size) {
            if (!domain_expansion(&buffer, &size)){
                status = CSV_ERROR_FILE;
                return result_out(status);
            }
        }
        buffer[len] = (char)c;
        len++;
        c = fgetc(reader->file);
    }
    buffer[len] = '\0';
     /*
    * Если произошла ошибка при чтении файла, освобождаем буфер и возвращаем результат с ошибкой CSV_ERROR_FILE.
    */
    if (ferror(reader->file)){
        free(buffer);
        CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
        return result;
    }
    /*
    * Если достигнут конец файла, проверяем, не было ли ошибок при чтении.
    */
    if (c == EOF){
        if (has_quotes && !closed_quotes){
            status = CSV_ERROR_FORMAT;
        } else {
            status = CSV_EOF;
        }
    }
    /*
    * Обновляем состояние читателя в зависимости от статуса.
    */
    if (status == CSV_ERROR_FORMAT){
        free(buffer);
        CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FORMAT};
        return result;
    }
    reader->in_record = (status == CSV_READ_END || status == CSV_EOF);
    CSVfieldresult result = {buffer, len, has_quotes, status};
    return result;
}