#include "fault.h"
#include "args.h"

int main(int argc, char *argv[]) {
    Args args = {0};
    ArgsStatus st = parse_args(argc, argv, &args);

    if (st == ARGS_HELP){
        args_help_print();
        return EXIT_OK;
    }
    if (st == ARGS_ERROR){
        return status(EXIT_ERROR_ARG, "Ошибка аргументов.");
    }
    
    switch(args.command){
        case COMMAND_SCHEMA:
            return args_schema_print(&args);
        case COMMAND_STATS:
            return args_stats_print(&args);
        case COMMAND_REPORT:
            return status(EXIT_ERROR_ARG, "Команда report не реализована");
        default:
            return status(EXIT_ERROR_ARG, "Неизвестная команда.");
    }
}