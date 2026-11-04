#ifndef ARGS_H
#define ARGS_H

#include "csv_reader.h"

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
    COMMAND_NELP,
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
#endif /* ARGS_H */