#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Forward slashes avoid the "\x", "\h", "\a" escape-sequence problem */
#define BOOKS_FILE  "C:/xampp/htdocs/assignment/books.txt"
#define RETURN_FILE "C:/xampp/htdocs/assignment/returned.txt"
#define PENALTY     14

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
            /* keep each record on one line and protect the '|' separator */
            for (char *c = out; *c; c++) {
                if (*c == '\n' || *c == '\r') *c = ' ';
                if (*c == '|') *c = '_';
            }
            return;
        }
        p += klen;
    }
}

static void print_escaped(const char *s) {
    for (; *s; s++) {
        if (*s == '<') printf("&lt;");
        else if (*s == '>') printf("&gt;");
        else if (*s == '&') printf("&amp;");
        else if (*s == '"') printf("&quot;");
        else putchar(*s);
    }
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n'))
        *--e = 0;
    return s;
}

/* Turn "YYYY-MM-DD" into a time_t (set to noon to avoid daylight-saving issues) */
static int parse_date(const char *s, time_t *out) {
    int y, m, d;
    if (sscanf(s, "%d-%d-%d", &y, &m, &d) != 3) return 0;
    if (y < 1970 || m < 1 || m > 12 || d < 1 || d > 31) return 0;

    struct tm t;
    memset(&t, 0, sizeof t);
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_hour = 12;
    t.tm_isdst = -1;

    time_t r = mktime(&t);
    if (r == (time_t)-1) return 0;
    *out = r;
    return 1;
}

static time_t today(void) {
    time_t now = time(NULL);
    struct tm t = *localtime(&now);
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    return mktime(&t);
}

/* Looks in books.txt (lines like "id | name | author").
   Returns 1 if found, 0 if not found, -1 if the file can't be opened. */
static int find_book(const char *id, char *name_out, size_t sz) {
    FILE *f = fopen(BOOKS_FILE, "r");
    if (f == NULL) return -1;

    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *a = strchr(line, '|');
        if (a == NULL) continue;
        *a = 0;
        char *rest = a + 1;
        char *b = strchr(rest, '|');
        if (b) *b = 0;

        if (strcmp(trim(line), id) == 0) {
            strncpy(name_out, trim(rest), sz - 1);
            name_out[sz - 1] = 0;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
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

    char bookid[64], duedate[32], retdate[32], bookname[256];
    get_field(data, "bookid", bookid, sizeof bookid);
    get_field(data, "duedate", duedate, sizeof duedate);
    get_field(data, "returndate", retdate, sizeof retdate);   /* optional */

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    time_t due, ret;

    if (bookid[0] == 0 || duedate[0] == 0) {
        printf("<p>Please enter the Book ID and the due date.</p>");
    }
    else if (!parse_date(duedate, &due)) {
        printf("<p>Invalid due date.</p>");
    }
    else if (retdate[0] != 0 && !parse_date(retdate, &ret)) {
        printf("<p>Invalid return date.</p>");
    }
    else {
        if (retdate[0] == 0) ret = today();      /* no date given: use today */

        int found = find_book(bookid, bookname, sizeof bookname);
        if (found == -1) {
            printf("<p>Error: could not open " BOOKS_FILE "</p>");
        }
        else if (found == 0) {
            printf("<p>No book with ID ");
            print_escaped(bookid);
            printf(" was found.</p>");
        }
        else {
            double x = difftime(ret, due) / 86400.0;
            long days_late = (long)(x + (x >= 0 ? 0.5 : -0.5));
            int penalty = (days_late > 0) ? PENALTY : 0;

            FILE *fptr = fopen(RETURN_FILE, "a");
            if (fptr != NULL) {
                fprintf(fptr, "%s | %s | due %s | returned %s | penalty %d\n",
                        bookid, bookname, duedate,
                        retdate[0] ? retdate : "today", penalty);
                fclose(fptr);
            }

            printf("<p>Book returned: ");
            print_escaped(bookname);
            printf(" (ID ");
            print_escaped(bookid);
            printf(")</p>");

            if (days_late > 0) {
                printf("<p>The book is %ld day(s) overdue. "
                       "A penalty of $%d must be paid.</p>", days_late, PENALTY);
            } else {
                printf("<p>Returned on time. No penalty.</p>");
            }
        }
    }

    printf("<p><a href=\"/assignment/returnbook.html\">Back to return form</a></p>");
    printf("</body></html>");
    return 0;
}
