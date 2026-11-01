#include "csv_reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


struct CSVReader {
    FILE *file;
    CSVoptions options;
    bool in_record;
};

// Создает новый объект CSVReader для чтения CSV-файла.
CSVReader *csv_reader_create(const char *filename, CSVoptions options){
    if (!filename){
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

// Освобождает ресурсы для конкретного результата чтения поля CSV.
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
    return CSV_RESET_OK;
}

// Читает следующее поле CSV из объекта CSVReader.
CSVfieldresult csv_reader_read_field(CSVReader *reader){
    assert(reader != NULL);
    assert(reader->file != NULL);

    size_t len = 0, size = 4;
    char *S = malloc(size);
    if (!S) {
        CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
        return result;
    }
    int c = fgetc(reader->file);
    if (c == EOF && reader->in_record){
        free(S);
        if (ferror(reader->file)){
            CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
            return result;
        }
        CSVfieldresult result = {NULL, 0 , false, CSV_EOF};
        return result;
    }
    bool has_quotes = false;
    bool closed_quotes = false;
    if (c == '"'){
        has_quotes = true;
        c = fgetc(reader->file);
    }
    while (c != EOF){ 
        if (has_quotes){
            if (c == '"'){
                int next_c = fgetc(reader->file);
                if (next_c == '"'){
                    c = '"';
                } else {
                    c = next_c;
                    closed_quotes = true;
                    break;
                }
            }
        } else{
            if (c == (unsigned char)reader->options.delimiter || c == '\n'){
                break;
            }
            if (c == '\r'){
                int next_c = fgetc(reader->file);
                if (next_c == '\n'){
                    c = '\n';
                    break;
                } else if (next_c != EOF){
                    if (ungetc(next_c, reader->file) == EOF){
                        free(S);
                        CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
                        return result;
                    }
                }                
            }
        }
        if (len + 1 == size){
            size *= 2;
            char *tmp = realloc(S, size);
            if (!tmp) {
                free(S);
                CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
                return result;
            }
            S = tmp;
        }
        S[len] = (char)c;
        len++;
        c = fgetc(reader->file);
    }
    S[len] = '\0';
    
    
    CSVstatus status = CSV_OK;
    if (c == EOF) {
        if (ferror(reader->file)) {
            free(S);
            CSVfieldresult result = {NULL, 0, false, CSV_ERROR_FILE};
            return result;
        }
        if (has_quotes && !closed_quotes) {
            status = CSV_ERROR_FORMAT;
        } else {
            status = CSV_EOF;
        }
    } else if (c == '\n' ) {
        status = CSV_READ_END;
    } else if (c == (unsigned char)reader->options.delimiter) {
        status = CSV_OK;
    } else if (has_quotes && !closed_quotes) {
        status = CSV_ERROR_FORMAT;
    } 

    if (status == CSV_READ_END || status == CSV_EOF){
        reader->in_record = true;
    } else{
        reader->in_record = false;
    }

    CSVfieldresult result = {S, len, has_quotes, status};
    return result;
}