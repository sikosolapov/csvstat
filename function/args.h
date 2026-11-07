#ifndef ARGS_H
#define ARGS_H

#include "csv_reader.h"
#include "field_type.h"
#include <stddef.h>

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
/*
* Для Stats
* Для удобства структуры, чтобы вывод сделать покайфу
*/
typedef struct {
    char *name;
    FieldType type;
    size_t nulls;
    size_t count;
    // Для INT
    bool start_int; // Для корректной работы max, min
    long long max_int, min_int;
    double sum_int; 
    size_t even; // число четных значений
    size_t positive_int, negative_int;
    // Для DOUBLE
    bool start_dbl; // Для корректной работы max, min
    double max_dbl, min_dbl;
    double sum_dbl;
    size_t integer; // число целыъ 
    size_t positive_dbl, negative_dbl;
    // Для STRING
    bool start_str; // Для корректной работы max, min
    size_t max_len, min_len;
    size_t sum_len;
    size_t empty;
} statistics_info;
int args_stats_print(const Args *args);
/*
*
*/
int args_report_print(const Args *args);
#endif /* ARGS_H */