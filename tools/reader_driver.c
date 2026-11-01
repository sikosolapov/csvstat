#include <stdio.h>
#include "csv_reader.h"

static const char *status_name(CSVstatus s) {
    switch (s) {
        case CSV_OK:           return "OK";
        case CSV_READ_END:     return "READ_END";
        case CSV_EOF:          return "EOF";
        case CSV_ERROR_FILE:   return "ERROR_FILE";
        case CSV_ERROR_FORMAT: return "ERROR_FORMAT";
    }
    return "?";
}

/* Один проход по файлу. Возвращает 0, если дошли до EOF, и 1 при ошибке. */
static int run_pass(CSVReader *reader) {
    for (;;) {
        CSVfieldresult f = csv_reader_read_field(reader);

        printf("%-12s len=%zu quotes=%d ",
               status_name(f.status), f.length, (int)f.has_quotes);
        if (f.data == NULL) {
            printf("data=(null)\n");
        } else {
            printf("data=[");
            fwrite(f.data, 1, f.length, stdout);
            printf("]\n");
        }

        CSVstatus st = f.status;
        csv_field_free(&f);

        if (st == CSV_EOF) {
            return 0;
        }
        if (st == CSV_ERROR_FILE || st == CSV_ERROR_FORMAT) {
            return 1;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s file.csv [twice]\n", argv[0]);
        return 2;
    }

    CSVoptions options = {',', false};
    CSVReader *reader = csv_reader_create(argv[1], options);
    if (reader == NULL) {
        fprintf(stderr, "cannot create reader for %s\n", argv[1]);
        return 1;
    }

    int rc = run_pass(reader);

    if (rc == 0 && argc == 3) {
        if (csv_reader_reset(reader) != CSV_RESET_OK) {
            fprintf(stderr, "reset failed\n");
            csv_reader_destroy(reader);
            return 1;
        }
        printf("--- second pass ---\n");
        rc = run_pass(reader);
    }

    csv_reader_destroy(reader);
    return rc;
}
