#include "args.h"
#include "csv_reader.h"
#include "field_type.h"
#include "fault.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
/*
* Для больших значения INT
*/
static void correct_sum_int(long long *x, unsigned long long *y, long long z){
    unsigned long long z1 = (unsigned long long)z;
    unsigned long long y0 = *y;
    *y += z1;
    if (*y < y0){
        (*x)++;
    }
    if (z < 0){
        (*x)--;
    }
}
static double correct_mean_int(long long x, unsigned long long y, size_t cnt){
    if (cnt == 0) return 0.0;
    long double SUM = (long double)x * 18446744073709551616.0L + (long double)y;
    return (double)(SUM / (long double)cnt);
}
/*
* Очистка, чтобы снова не делать ctrl-c, ctrl-v. Лень залог успеха
*/
static void report_free(statistics_info *information, size_t capacity){
    if (!information) return;
    for (size_t Y = 0; Y < capacity; Y++){
        free(information[Y].name);
    }
    free(information);
}
static int error_report(CSVReader *reader, statistics_info *information, size_t columns, 
    CSVfieldresult *field, const char *msg, FILE *output, const char *output_f){
    if (output){
        fclose(output);
        remove(output_f);
    }
    csv_field_free(field);
    report_free(information, columns);
    csv_reader_destroy(reader);
    return status(EXIT_ERROR_FILE,  msg);
}
/*
* Снова для прекрасной жизни используем "расширение территории" 
*/
static bool report_expansion(statistics_info **info, size_t *columns){
    size_t cap = *columns;
    size_t new_cap = (cap == 0) ? 4 : cap * 2;
    statistics_info *tmp = realloc(*info, new_cap * sizeof(statistics_info));
    if (!tmp){
        return false;
    }
    memset(tmp + cap, 0, (new_cap - cap) * sizeof(statistics_info));
    *info = tmp;
    *columns = new_cap;
    return true;
}
/*
* Основная функция stats с выводом
*/
int args_report_print(const Args *args){
    assert(args != NULL && args->input_file != NULL);
    CSVoptions option = {
        .delimiter = args->delimiter,
        .multiline = args->multiline
    };
    CSVReader *reader = csv_reader_create(args->input_file, option);
    if (!reader){
        return status(EXIT_ERROR_FILE, "Ошибка: Открытия файла");
    }
    FILE *output = fopen(args->output_file, "w");
    if (!output){
        csv_reader_destroy(reader);
        return status(EXIT_ERROR_FILE, "Ошибка: Вывода статистики в файл...");
    }
    statistics_info *information = NULL;
    bool header = true;         // Определяет смотрим мы на заголовок или нет
    size_t num_column = 0;      // Номер столбца смотрим
    size_t columns = 0;         // Количество колонок
    size_t rows = 0;            // Количество строчек
    size_t capacity = 0;        // Размер массива с данными
    while(true){
        CSVfieldresult field = csv_reader_read_field(reader);
        if (field.status == CSV_EOF && field.data == NULL){
            if (header){
                return error_report(reader, information, num_column, &field, "Ошибка: Пустой файл", output, args->output_file);
            }
            break;
        }
        if (field.status == CSV_ERROR_FILE || field.status == CSV_ERROR_FORMAT){
            return error_report(reader, information, header? num_column: columns, &field, "Ошибка: Файла/формата",output, args->output_file);
        }
        if (field.data != NULL){
            field.length = field_check_and_cut(field.data, field.has_quotes);
        }
        if (header){
            if (field.data == NULL || field.length == 0){
                return error_report(reader, information, num_column, &field, "Ошибка: Нет названия", output, args->output_file);
            }
            for (size_t j = 0; j < num_column; j++){
                if (strcmp(information[j].name, field.data) == 0){
                    return error_report(reader, information, header ? num_column : columns, &field, "Ошибка: Повтор имени", output, args->output_file);
                }
            }
            if (num_column >= capacity && !report_expansion(&information, &capacity)){
                return error_report(reader, information, num_column, &field, "Ошибка: При выделении памяти", output, args->output_file);
            }
            information[num_column].name = field.data;
            information[num_column].type = FIELD_TYPE_NULL;
            field.data = NULL; // теперь не нужна, ведь ей не принадлежит
            num_column++;
        } else {
            if (num_column >= columns){
                return error_report(reader, information, columns, &field, "Ошибка: В строке больше полей, чем в заголовке", output, args->output_file);
            }
            FieldType type_field = FIELD_TYPE_NULL;
            if (field.data != NULL){
                type_field = type_inference(field.data, field.has_quotes, args->scientific);
            }
            if (type_field > information[num_column].type){
                information[num_column].type = type_field;
            }
            /*
            * Обновление статистики
            */
            if (type_field == FIELD_TYPE_NULL){
                information[num_column].nulls++;
            } else{ 
                information[num_column].count++;
                if (type_field == FIELD_TYPE_INT){
                    long long number = strtoll(field.data, NULL, 10);
                    if (!information[num_column].start_int){
                        information[num_column].max_int = number;
                        information[num_column].min_int = number;
                        information[num_column].start_int = true;
                    } else {
                        if (number > information[num_column].max_int){
                            information[num_column].max_int = number;
                        } 
                        if (number < information[num_column].min_int){
                            information[num_column].min_int = number;
                        }
                    }
                    correct_sum_int(&information[num_column].x, &information[num_column].y, number);
                    if (number % 2 == 0) information[num_column].even++;
                    if (number > 0) information[num_column].positive_int++;
                    if (number < 0) information[num_column].negative_int++;
                    double dbl = (double)number;
                    if (!information[num_column].start_dbl){
                        information[num_column].max_dbl = dbl;
                        information[num_column].min_dbl = dbl;
                        information[num_column].start_dbl = true;
                    } else {
                        if (dbl > information[num_column].max_dbl){
                            information[num_column].max_dbl = dbl;
                        } 
                        if (dbl < information[num_column].min_dbl){
                            information[num_column].min_dbl = dbl;
                        }
                    }
                    information[num_column].sum_dbl += dbl;
                    information[num_column].integer++;
                    if (dbl > 0) information[num_column].positive_dbl++;
                    if (dbl < 0) information[num_column].negative_dbl++;
                } else if (type_field == FIELD_TYPE_DOUBLE){
                    double number = strtod(field.data, NULL);
                    if (!information[num_column].start_dbl){
                        information[num_column].max_dbl = number;
                        information[num_column].min_dbl = number;
                        information[num_column].start_dbl = true;
                    } else {
                        if (number > information[num_column].max_dbl){
                            information[num_column].max_dbl = number;
                        } 
                        if (number < information[num_column].min_dbl){
                            information[num_column].min_dbl = number;
                        }
                    }
                    information[num_column].sum_dbl += number;
                    if (trunc(number) == number) information[num_column].integer++;
                    if (number > 0) information[num_column].positive_dbl++;
                    if (number < 0) information[num_column].negative_dbl++;
                } 
                if (type_field != FIELD_TYPE_NULL){
                    size_t len = field.length;
                    if (!information[num_column].start_str){
                        information[num_column].max_len = len;
                        information[num_column].min_len = len;
                        information[num_column].start_str = true;
                    } else {
                        if (len > information[num_column].max_len){
                            information[num_column].max_len = len;
                        } 
                        if (len < information[num_column].min_len){
                            information[num_column].min_len = len;
                        }
                    }
                    information[num_column].sum_len += len;
                    if (len == 0) information[num_column].empty++;
                }
            }
            num_column++;
        }
        bool row_status = (field.status == CSV_READ_END || field.status == CSV_EOF); // Для строк статус
        if (row_status){
            if (header){
                columns = num_column;
                header = false;
                num_column = 0;
            } else {
                if (num_column != columns){
                    return error_report(reader, information, header ? num_column : columns, &field, "Ошибка: Недостаточно полей", output, args->output_file);
                }
                rows++;
                num_column = 0;
            }
        }
        csv_field_free(&field);
        if (field.status == CSV_EOF) break;
    }
    /*
    * Вывод
    * Нет внешних границ, тк мне лень их делать. Ну и нет внешних границ -> нет с ними проблем
    * Нужно тогда выравнивание слева, но зачем его делать, если можно не делать...
    */
    fprintf(output,"\n");
    fprintf(output,"\n");
    fprintf(output,"\n");
    fprintf(output, "CSVSTAT Report\n");
    fprintf(output, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    fprintf(output, "Input-File %s\n", args->input_file);
    fprintf(output, "Rows %zu\n", rows);
    fprintf(output, "Columns %zu\n", columns);
    fprintf(output, "Table -- Stats\n"); // Для красоты
    fprintf(output, "--------+------------+----------------------+------------------------------------------\n");
    fprintf(output, " Index  | Column Name|         Type         |               Statistics                 \n");
    fprintf(output, "--------+------------+----------------------+------------------------------------------\n");
    for (size_t qwe = 0; qwe < columns; qwe++){
        const char *type = field_type_for_print(information[qwe].type);
        fprintf(output, "%-7zu | %-10s | %-20s | Nulls: %zu\n", qwe, information[qwe].name, type, information[qwe].nulls);
        switch (information[qwe].type){
        case FIELD_TYPE_NULL:
            break;
        case FIELD_TYPE_INT:
            fprintf(output, "        |            |                      | Min: %lld\n", information[qwe].min_int);
            fprintf(output, "        |            |                      | Max: %lld\n", information[qwe].max_int);
            fprintf(output, "        |            |                      | Mean: %.6f\n", correct_mean_int(information[qwe].x, information[qwe].y, information[qwe].count));
            fprintf(output, "        |            |                      | Positive: %zu\n", information[qwe].positive_int);
            fprintf(output, "        |            |                      | Negative: %zu\n", information[qwe].negative_int);
            fprintf(output, "        |            |                      | Even: %zu\n", information[qwe].even);
            break;
        case FIELD_TYPE_DOUBLE: {
            double min = information[qwe].min_dbl;
            double max = information[qwe].max_dbl;
            double mean = information[qwe].count != 0 ? information[qwe].sum_dbl / information[qwe].count : 0.0;
            if (min == 0.0) min = 0.0;
            if (max == 0.0) max = 0.0;
            if (mean == 0.0) mean = 0.0;
            fprintf(output, "        |            |                      | Min: %.6f\n", min);
            fprintf(output, "        |            |                      | Max: %.6f\n", max);
            fprintf(output, "        |            |                      | Mean: %.6f\n", mean);
            fprintf(output, "        |            |                      | Positive: %zu\n", information[qwe].positive_dbl);
            fprintf(output, "        |            |                      | Negative: %zu\n", information[qwe].negative_dbl);
            fprintf(output, "        |            |                      | Integer: %zu\n", information[qwe].integer);
            break;
        }
        case FIELD_TYPE_STRING:
            fprintf(output, "        |            |                      | Min: %zu\n", information[qwe].min_len);
            fprintf(output, "        |            |                      | Max: %zu\n", information[qwe].max_len);
            fprintf(output, "        |            |                      | Mean: %.6f\n", information[qwe].count != 0 ? (double)information[qwe].sum_len / information[qwe].count : 0.0);
            fprintf(output, "        |            |                      | Empty: %zu\n", information[qwe].empty);
            break;
        default:
            assert(0 && "Неизвестный тип поля");
            break;
        }
       fprintf(output, "--------+------------+----------------------+------------------------------------------\n");
    }
    fprintf(output,"\n");
    fprintf(output,"\n");
    fprintf(output,"\n");
    /*
    * Обработка ошибок
    */
    if (ferror(output)){
        fclose(output);
        remove(args->output_file);
        report_free(information, columns);
        csv_reader_destroy(reader);
        return status(EXIT_ERROR_FILE, "Ошибка: При записи вывода в файл");
    }
    fclose(output);
    /*
    * Очистка
    */
    report_free(information, columns);
    csv_reader_destroy(reader);
    return EXIT_OK;
}