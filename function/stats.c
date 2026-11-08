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
static void stats_free(statistics_info *information, size_t capacity){
    if (!information) return;
    for (size_t Y = 0; Y < capacity; Y++){
        free(information[Y].name);
    }
    free(information);
}
static int error_stats(CSVReader *reader, statistics_info *information, size_t columns, 
    CSVfieldresult *field, const char *msg){
    csv_field_free(field);
    stats_free(information, columns);
    csv_reader_destroy(reader);
    return status(EXIT_ERROR_FILE,  msg);
}
/*
* Снова для прекрасной жизни используем "расширение территории" 
*/
static bool stats_expansion(statistics_info **info, size_t *columns){
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
int args_stats_print(const Args *args){
    assert(args != NULL && args->input_file != NULL);
    CSVoptions option = {
        .delimiter = args->delimiter,
        .multiline = args->multiline
    };
    CSVReader *reader = csv_reader_create(args->input_file, option);
    if (!reader){
        return status(EXIT_ERROR_FILE, "Ошибка: Открытия файла");
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
                return error_stats(reader, information, num_column, &field, "Ошибка: Пустой файл");
            }
            break;
        }
        if (field.status == CSV_ERROR_FILE || field.status == CSV_ERROR_FORMAT){
            return error_stats(reader, information, header? num_column: columns, &field, "Ошибка: Файла/формата");
        }
        if (field.data != NULL){
            field.length = field_check_and_cut(field.data, field.has_quotes);
        }
        if (header){
            if (field.data == NULL || field.length == 0){
                return error_stats(reader, information, num_column, &field, "Ошибка: Нет названия");
            }
            for (size_t j = 0; j < num_column; j++){
                if (strcmp(information[j].name, field.data) == 0){
                    return error_stats(reader, information, header ? num_column : columns, &field, "Ошибка: Повтор имени");
                }
            }
            if (num_column >= capacity && !stats_expansion(&information, &capacity)){
                return error_stats(reader, information, num_column, &field, "Ошибка: При выделении памяти");
            }
            information[num_column].name = field.data;
            information[num_column].type = FIELD_TYPE_NULL;
            field.data = NULL; // теперь не нужна, ведь ей не принадлежит
            num_column++;
        } else {
            if (num_column >= columns){
                return error_stats(reader, information, columns, &field, "Ошибка: В строке больше полей, чем в заголовке");
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
                    return error_stats(reader, information, header ? num_column : columns, &field, "Ошибка: Недостаточно полей");
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
    */
    printf("rows %zu\n", rows);
    printf("columns %zu\n", columns);
    for (size_t t = 0; t < columns; t++){
        if (t > 0) printf("\n");
        printf("column %zu\n", t);
        printf("name \"");
        for (const char *s = information[t].name; *s != '\0'; s++){
            if (*s == '"') putchar('"');
            putchar(*s);
        }
        printf("\"\n");
        printf("type %s\n", field_type_for_print(information[t].type));
        switch (information[t].type){
        case FIELD_TYPE_NULL:
            printf("rows %zu\n", rows);
            break;
        case FIELD_TYPE_INT:
            printf("nulls %zu\n", information[t].nulls);
            printf("min %lld\n", information[t].min_int);
            printf("max %lld\n", information[t].max_int);
            printf("mean %.6f\n", correct_mean_int(information[t].x, information[t].y, information[t].count));
            printf("positive %zu\n", information[t].positive_int);
            printf("negative %zu\n", information[t].negative_int);
            printf("even %zu\n", information[t].even);
            break;
        case FIELD_TYPE_DOUBLE: {
            double min = information[t].min_dbl;
            double max = information[t].max_dbl;
            double mean = information[t].count != 0 ? information[t].sum_dbl / information[t].count : 0.0;
            if (min == 0.0) min = 0.0;
            if (max == 0.0) max = 0.0;
            if (mean == 0.0) mean = 0.0;
            printf("nulls %zu\n", information[t].nulls);
            printf("min %.6f\n", min);
            printf("max %.6f\n", max);
            printf("mean %.6f\n", mean);
            printf("positive %zu\n", information[t].positive_dbl);
            printf("negative %zu\n", information[t].negative_dbl);
            printf("integer %zu\n", information[t].integer);
            break;
        }
        case FIELD_TYPE_STRING:
            printf("nulls %zu\n", information[t].nulls);
            printf("min_length %zu\n", information[t].min_len);
            printf("max_length %zu\n", information[t].max_len);
            printf("mean_length %.6f\n", information[t].count != 0 ? (double)information[t].sum_len / information[t].count : 0.0);
            printf("empty %zu\n", information[t].empty);
            break;
        default:
            assert(0 && "Неизвестный тип поля");
            break;
        }
    }
    /*
    * Очистка
    */
    stats_free(information, columns);
    csv_reader_destroy(reader);
    return EXIT_OK;
}