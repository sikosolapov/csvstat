#include "args.h"
#include "csv_reader.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void args_help_print(void){
    fputs(
"Usage:\n"
"  csvstat --help\n"
"  csvstat schema <csv-file> [options]\n"
"  csvstat stats  <csv-file> [options]\n"
"  csvstat report <csv-file> <output-file> [options]\n"
"\nOptions:\n"
"  --delimiter <d>   column delimiter (default ',')\n"
"  --multiline       allow newlines inside quoted fields\n"
"  --scientific      use scientific notation for floats\n"
"\nExit codes: 0 ok, 1 file/input/exec error, 2 command-line error.\n",
    stdout);
}

ArgsStatus parse_args(int argc, char *argv[], Args *args){
    assert(args != NULL);
    /*
    * Начальное состояние
    */
    args->command = COMMAND_NONE;
    args->delimiter = ',';
    args->delimiter_status = false;
    args->multiline = args->multiline_status = false;
    args->scientific = args->scientific_status = false;
    args->input_file = args->output_file = NULL;
    /*
    * Выполнение - Основной часть
    */
    for(int i = 1 ; i < argc; ++i){
        const char *w = argv[i];
        /*
        * Основные команды - help, --delimiter, multiline, scientific.
        * С защитой от повтора
        */
        if (w[0] == '-' && w[1] == '-'){
            if (strcmp(w, "--help") == 0){
                if (argc != 2) return ARGS_ERROR;
                return ARGS_HELP;
            }
            if (strcmp(w, "--delimiter") == 0){
                if (args->delimiter_status) return ARGS_ERROR;
                if (i + 1 >= argc) return ARGS_ERROR;
                const char *v = argv[i+1]; char c = v[0];
                if (strlen(v) != 1 || !csv_delimiter_check(c)){
                    return ARGS_ERROR;
                }
                args->delimiter = c; args->delimiter_status = true;
                i++;
                continue;
            }
            if (strcmp(w, "--multiline") == 0){
                if (args->multiline_status) return ARGS_ERROR;
                args->multiline = true; args->multiline_status = true;
                continue;
            }
             if (strcmp(w, "--scientific") == 0){
                if (args->scientific_status) return ARGS_ERROR;
                args->scientific = true; args->scientific_status = true;
                continue;
            }
            /*
            * При условие, что введено что-то другое
            */
            return ARGS_ERROR;
        }
        /*
        * Пользователь снова шалит
        */
        if (w[0] == '-'){
            return ARGS_ERROR;
        }
        /*
        * Обработка команд. Ну и файлов
        */
        if (args->command == COMMAND_NONE){
            if (strcmp(w, "schema") == 0) args->command = COMMAND_SCHEMA;
            else if (strcmp(w, "report") == 0) args->command = COMMAND_REPORT;
            else if (strcmp(w, "stats") == 0)args->command = COMMAND_STATS;
            else return ARGS_ERROR;
            continue;
        }
        if (args->input_file == NULL){
            args->input_file = w;
            continue;
        }
        if (args->output_file == NULL){
            args->output_file = w;
            continue;
        }
        return ARGS_ERROR;
    }
    if (args->command == COMMAND_NONE) return ARGS_ERROR;
    if (args->input_file == NULL) return ARGS_ERROR;
    if (args->command == COMMAND_REPORT && args->output_file == NULL) return ARGS_ERROR;
    if (args->command != COMMAND_REPORT && args->output_file != NULL) return ARGS_ERROR;
    return ARGS_OK;
}