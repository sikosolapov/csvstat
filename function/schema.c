#include "args.h"
#include "csv_reader.h"
#include "field_type.h"
#include "fault.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
* Очистка, чтобы снова не делать ctrl-c, ctrl-v. Лень залог успеха
*/
static void schema_stats_free(schema_stats *stats){
    if (!stats) return;
    if (stats->columns_st){
        for (size_t k = 0; k < stats->columns; k++){
            free(stats->columns_st[k].name);
        }
        free(stats->columns_st);
        stats->columns_st = NULL;
    }
    stats->columns = 0;
    stats->rows = 0;
}
static int ERROR_schema(CSVReader *reader, schema_stats *stats, CSVfieldresult *field, const char *msg){
    csv_field_free(field);
    schema_stats_free(stats);
    csv_reader_destroy(reader);
    return status(EXIT_ERROR_FILE,  msg);
}
/*
* Снова для прекрасной жизни используем "расширение территории" 
*/
static bool schema_expansion(schema_column **columns, size_t *size){
    size_t cap = (*size == 0) ? 4: *size * 2;
    schema_column *tmp = realloc(*columns, cap * sizeof(schema_column));
    if (!tmp){
        return false;
    }
    *columns = tmp;
    *size = cap;
    return true;
}
/*
* Сама функция schema
*/
int args_schema_print(const Args *args){
    assert(args != NULL && args->input_file != NULL);
    /*
    *  что-то точно хочу написать
    */
    CSVoptions option = {
        .delimiter = args->delimiter,
        .multiline = args->multiline
    };
    CSVReader *reader = csv_reader_create(args->input_file, option);
    if (!reader){
        return status(EXIT_ERROR_FILE, "Ошибка открытия файла");
    }
    schema_stats statistics = {0};
    bool header = true;         // Кентик для заголовков
    size_t num_column = 0;      // Кентик для номера столбца
    size_t capacity = 0;        // Кентик для размера массивчика
    while(true){
        CSVfieldresult field = csv_reader_read_field(reader);
        if (field.status == CSV_ERROR_FILE || field.status == CSV_ERROR_FORMAT){
            return ERROR_schema(reader, &statistics, &field, "Ошибка определения типа файла. Ошибка файла");
        }
        if (field.data != NULL){
            field.length = field_check_and_cut(field.data, field.has_quotes);
        }
        if (header){
            if (field.data == NULL || field.length == 0){
                return ERROR_schema(reader, &statistics, &field, "Нет имени столбца");
            }
            for (size_t q = 0; q < num_column; q++){
                if (strcmp(statistics.columns_st[q].name, field.data) == 0){
                    return ERROR_schema(reader, &statistics, &field, "Повтор имени. Поменяй, если хочешь");
                }
            }
            if (num_column >= capacity && !schema_expansion(&statistics.columns_st, &capacity)){
                return ERROR_schema(reader, &statistics, &field, "Ошибка: Что-то с выделением памяти");
            }
            statistics.columns_st[num_column].number = num_column;
            statistics.columns_st[num_column].name = field.data;
            statistics.columns_st[num_column].type = FIELD_TYPE_NULL;
            field.data = NULL; // теперь не нужна, ведь ей не принадлежит
            num_column++;
        } else {
            if (num_column >= statistics.columns){
                return ERROR_schema(reader, &statistics, &field, "Ошибка: Почему номер столбца больше их количества...");
            }
            FieldType type_field = type_inference(field.data, field.has_quotes, args->scientific);
            if (type_field > statistics.columns_st[num_column].type){
                statistics.columns_st[num_column].type = type_field;
            }
            num_column++;
        }
        bool row_status = (field.status == CSV_READ_END || field.status == CSV_EOF); // Для строк статус
        csv_field_free(&field);
        if (row_status){
            if (header){
                statistics.columns = num_column;
                header = false;
            } else {
                if (num_column != statistics.columns){
                    return ERROR_schema(reader, &statistics, NULL, "Есть чутка несовпадения");
                }
                statistics.rows++;
            }
            num_column = 0;
        }
         if (field.status == CSV_EOF) break;
    }
    if (statistics.columns == 0){
        return ERROR_schema(reader, &statistics, NULL, "Файл какой-то пустой");
    }
    printf("rows: %zu\n", statistics.rows);
    printf("columns: %zu\n", statistics.columns);
    for (size_t W = 0; W < statistics.columns; W++){
        const char *name_type = field_type_for_print(statistics.columns_st[W].type);
        printf("%zu \"", W);
        for (size_t H = 0; statistics.columns_st[W].name[H] != '\0';H++){
            if (statistics.columns_st[W].name[H] == '"'){
                putchar('"');
            }
            putchar(statistics.columns_st[W].name[H]);
        }
        printf("\" %s\n", name_type);
    }
    /*
    * Очистка
    */
    schema_stats_free(&statistics);
    csv_reader_destroy(reader);
    return EXIT_OK;
}