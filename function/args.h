#ifndef ARGS_H
#define ARGS_H

#include "csv_reader.h"

typedef enum {
    ARGS_OK = 0,
    ARGS_HELP = 1,
    ARGS_ERROR = 2,
    ARGS_COUNT 
} ArgsStatus;

typedef enum{
    COMMAND_NONE,
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
void args_command_help_print(Command command);
#endif /* ARGS_H */