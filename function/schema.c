#include "args.h"
#include "csv_reader.h"
#include "field_type.h"
#include "fault.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t number;
    char *name;
    FieldType type;
} schema_column;

typedef struct {
    size_t rows;
    size_t columns;
    schema_column *columns_st;
} schema_stats;


int args_schema_print(const Args *args){
    if (!args || !args->input_file){
        return status(EXIT_ERROR_FILE, "Нет файла");
    }
    CSVoptions option = {
        .delimiter = args->delimiter,
        .multiline = args->multiline
    };
    CSVReader *reader = csv_reader_create(args->input_file, option);
    if (!reader){
        return status(EXIT_ERROR_FILE, "Ошибка открытия файла");
    }
    schema_stats statistics = {0};


    free(statistics.columns_st);
    csv_reader_destroy(reader);
    
    return EXIT_OK;
}

