#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward slashes avoid the "\x", "\h", "\a" escape-sequence problem */
#define OUT_FILE "C:/xampp/htdocs/assignment/books.txt"

static void url_decode(char *dst, const char *src) {
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], 0 };
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

static void get_field(const char *data, const char *name, char *out, size_t outsz) {
    char key[64];
    snprintf(key, sizeof key, "%s=", name);
    size_t klen = strlen(key);
    const char *p = data;
    out[0] = 0;

    while ((p = strstr(p, key)) != NULL) {
        if (p == data || p[-1] == '&') {
            p += klen;
            size_t n = strcspn(p, "&");
            char *raw = malloc(n + 1);
            char *dec = malloc(n + 1);
            memcpy(raw, p, n);
            raw[n] = 0;
            url_decode(dec, raw);
            strncpy(out, dec, outsz - 1);
            out[outsz - 1] = 0;
            free(raw);
            free(dec);
            /* keep each record on one line */
            for (char *c = out; *c; c++)
                if (*c == '\n' || *c == '\r') *c = ' ';
            return;
        }
        p += klen;
    }
}

int main(void) {
    char data[4096] = "";
    char *method = getenv("REQUEST_METHOD");

    if (method && strcmp(method, "POST") == 0) {
        char *len = getenv("CONTENT_LENGTH");
        int n = len ? atoi(len) : 0;
        if (n > (int)sizeof(data) - 1) n = sizeof(data) - 1;
        size_t got = fread(data, 1, n, stdin);
        data[got] = 0;
    } else {
        char *q = getenv("QUERY_STRING");
        if (q) strncpy(data, q, sizeof(data) - 1);
    }

    char name[256], id[64], author[256];
    get_field(data, "bookname", name, sizeof name);
    get_field(data, "bookid", id, sizeof id);
    get_field(data, "author", author, sizeof author);

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    FILE *fptr = fopen(OUT_FILE, "a");
    if (fptr == NULL) {
        printf("<p>Error: could not open " OUT_FILE "</p>");
        printf("<p>Check that the folder exists.</p>");
    } else {
        fprintf(fptr, "%s | %s | %s\n", id, name, author);
        fclose(fptr);
        printf("<p>Book Name: %s</p><p>Book ID: %s</p><p>Author: %s</p>", name, id, author);
        printf("<p>Added successfully.</p>");
    }

    printf("<p><a href=\"/assignment/form.html\">Back to form</a></p>");
    printf("</body></html>");
    return 0;
}
