#ifndef ARGS_H
#define ARGS_H

#include "csv_reader.h"
#include "field_type.h"

typedef enum {
    ARGS_OK,
    ARGS_HELP,
    ARGS_ERROR,
    ARGS_COUNT 
} ArgsStatus;
/*
* Основные команды  
*/
typedef enum{
    COMMAND_NONE,
    COMMAND_HELP,
    COMMAND_STATS,
    COMMAND_SCHEMA,
    COMMAND_REPORT,
    COMMAND_COUNT
} Command;

typedef struct{
    Command command;
    const char *input_file;
    const char *output_file;
    char delimiter;
    bool scientific;
    bool multiline;

    bool delimiter_status;
    bool scientific_status;
    bool multiline_status;
} Args;

ArgsStatus parse_args(int argc, char *argv[], Args *args);
void args_help_print(void);
/*
* Для Schema
* Для удобства структуры, чтобы вывод сделать покайфу
*/
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
int args_schema_print(const Args *args);
int args_stats_print(const Args *args);
#endif /* ARGS_H */