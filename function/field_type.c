#include "field_type.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

size_t field_check_and_cut(char *s, bool has_quotes){
    assert(s != NULL);
    if (has_quotes) return strlen(s);
    /*
    * Правая и левая границы поля.
    */
    size_t left = 0;
    size_t right = strlen(s);
    while (left < right && (s[left] == ' ' || s[left] == '\t')){
        left ++;
    }
    while (right > left && (s[right-1] == ' ' || s[right-1] == '\t')){
        right--;
    }
    memmove(s, s+left, right - left);
    s[right-left] = '\0';
    return right - left;
}

FieldType type_inference(const char *s, bool has_quotes, bool scientific){
    size_t cur = 0;
    size_t int_digits = 0;
    size_t digits_after_dot = 0;
    bool dot = false;
    bool exponent = false;
    assert(s != NULL);
    if (!*s){
        return has_quotes ? FIELD_TYPE_STRING: FIELD_TYPE_NULL;
    }
    /*
    * Проверка на знак перед числом/строкой
    */
    if (s[cur] == '+' || s[cur] == '-') {
        cur++;
    }
    /*
    * Подсчет целой части
    */
    while (s[cur] >= '0' && s[cur] <= '9') {
        cur++;
        int_digits++;
    }
    /*
    * Подсчет цифр дробной части
    */
    if (s[cur] == '.') {
        dot = true;
        cur++;

        while (s[cur] >= '0' && s[cur] <= '9') {
            cur++;
            digits_after_dot++;
        }
    }
    /*
    * Определение типов после подсчета
    */
    if (int_digits == 0){
        return FIELD_TYPE_STRING;
    }
    if (dot && digits_after_dot == 0){
        return FIELD_TYPE_STRING;
    }
    if (s[cur] == 'e' || s[cur] == 'E'){
        if (!scientific){
            return FIELD_TYPE_STRING;
        }
        exponent = true;
        cur++;
        if (s[cur] == '+' || s[cur] == '-'){
            cur++;
        }
        size_t exponent_digits = 0;
        while (s[cur] >= '0' && s[cur] <= '9'){
            cur++;
            exponent_digits++;
        }
        if (exponent_digits == 0){
            return FIELD_TYPE_STRING;
        }
    }
    if (s[cur] != '\0'){
        return FIELD_TYPE_STRING;
    }
    if (dot || exponent) {
        double value = strtod(s, NULL);
        if (!isfinite(value)){
            return FIELD_TYPE_STRING;
        }
        return FIELD_TYPE_DOUBLE;
    }
    /*
    * Переменная для проверки диапазона числа.
    */
    const char *d = s;
    /*
    * Пропуск знака и ведущих нулей 
    */
    if (*d == '+' || *d == '-') d++;
    while (*d == '0') d++;
    if (*d == '\0') return FIELD_TYPE_INT; // Для нуля
    const char *limit;
    if (s[0] == '-') {
        limit = "9223372036854775808";
    } else {
        limit = "9223372036854775807";
    }
    size_t len_d = strlen(d);
    size_t len_limit = strlen(limit);
    if (len_d < len_limit) return FIELD_TYPE_INT;
    if (len_d > len_limit) return FIELD_TYPE_STRING;
    for (size_t k = 0; k < len_d; k++) {
        if (d[k] < limit[k]) return FIELD_TYPE_INT;
        if (d[k] > limit[k]) return FIELD_TYPE_STRING;
    }
    return FIELD_TYPE_INT;
}