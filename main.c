#include "fault.h"
#include "args.h"

int main(int argc, char *argv[]) {
    Args args;
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

            break;
        case COMMAND_STATS:

            break;
        case COMMAND_REPORT:

            break;
        default:
            return status(EXIT_ERROR_ARG, "Неизвестная команда.");
    }

    
    return EXIT_OK;
}