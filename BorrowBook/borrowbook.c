#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BOOKS_FILE    "C:/xampp/htdocs/assignment/books.txt"
#define BORROWED_FILE "C:/xampp/htdocs/assignment/borrowed.txt"
#define LOAN_DAYS 14     /* how many days the user can keep the book */
#define SEP '|'          /* character that separates the fields in books.txt */
#define MAX_LINES 200    /* books.txt can hold up to this many lines */

/* books.txt line format:  name|id|author|stock */

char lines[MAX_LINES][500];   /* all lines of books.txt are kept here */

/* turns %20 and + in the form data back into normal text */
void decode(char *out, char *in) {
    while (*in) {
        if (*in == '+') {
            *out = ' ';
            in++;
        } else if (*in == '%' && in[1] && in[2]) {
            char hex[3] = { in[1], in[2], 0 };
            *out = (char)strtol(hex, NULL, 16);
            in += 3;
        } else {
            *out = *in;
            in++;
        }
        out++;
    }
    *out = 0;
}

/* finds name=value in the form data and copies value into result */
void get_value(char *data, char *name, char *result) {
    char key[50];
    char *p;
    char temp[300];
    int len, i;

    result[0] = 0;
    sprintf(key, "%s=", name);
    p = strstr(data, key);
    if (p == NULL) return;

    p = p + strlen(key);
    len = strcspn(p, "&");
    if (len > 290) len = 290;
    strncpy(temp, p, len);
    temp[len] = 0;

    decode(result, temp);

    for (i = 0; result[i]; i++) {
        if (result[i] == SEP || result[i] == '\n' || result[i] == '\r')
            result[i] = ' ';
    }
}

/* removes spaces from the start and end of a word */
char *trim(char *s) {
    char *end;
    while (*s == ' ') s++;
    end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\n' || end[-1] == '\r')) {
        end--;
        *end = 0;
    }
    return s;
}

/* finds the book in books.txt, takes 1 from its stock and saves the file
   returns  1 = done
            0 = book not found
           -1 = file problem
           -2 = no stock left */
int take_stock(char *id, char *name, int *left) {
    FILE *fp;
    int count = 0;
    int i, n, stock, found = -1;
    char temp[500];
    char *f[4];
    char *p;

    /* read every line of books.txt into memory */
    fp = fopen(BOOKS_FILE, "r");
    if (fp == NULL) return -1;
    while (count < MAX_LINES && fgets(lines[count], 500, fp)) count++;
    fclose(fp);

    /* look for the line with this book id */
    for (i = 0; i < count; i++) {
        strcpy(temp, lines[i]);

        /* split the line into 4 parts: name, id, author, stock */
        n = 1;
        f[0] = temp;
        p = temp;
        while (*p && n < 4) {
            if (*p == SEP) {
                *p = 0;
                f[n] = p + 1;
                n++;
            }
            p++;
        }
        if (n < 4) continue;      /* not a full book line, skip it */

        f[0] = trim(f[0]);
        f[1] = trim(f[1]);
        f[2] = trim(f[2]);
        f[3] = trim(f[3]);

        if (strcmp(f[1], id) == 0) {
            strcpy(name, f[0]);
            stock = atoi(f[3]);

            if (stock <= 0) return -2;     /* nothing left to borrow */

            stock = stock - 1;
            *left = stock;

            /* build the new line with the smaller stock */
            sprintf(lines[i], "%s%c%s%c%s%c%d\n",
                    f[0], SEP, f[1], SEP, f[2], SEP, stock);
            found = i;
            break;
        }
    }

    if (found == -1) return 0;

    /* write all lines back into books.txt */
    fp = fopen(BOOKS_FILE, "w");
    if (fp == NULL) return -1;
    for (i = 0; i < count; i++) fputs(lines[i], fp);
    fclose(fp);
    return 1;
}

int main() {
    char data[2000] = "";
    char id[50], borrower[100], name[300];
    char borrow_date[20], due_date[20];
    char *method, *size_text;
    int size, result, left = 0;
    time_t now, due;
    struct tm t;
    FILE *fp;

    /* read the form data */
    method = getenv("REQUEST_METHOD");
    if (method != NULL && strcmp(method, "POST") == 0) {
        size_text = getenv("CONTENT_LENGTH");
        size = size_text ? atoi(size_text) : 0;
        if (size > 1990) size = 1990;
        size = fread(data, 1, size, stdin);
        data[size] = 0;
    } else {
        char *q = getenv("QUERY_STRING");
        if (q != NULL) strncpy(data, q, 1990);
    }

    get_value(data, "bookid", id);
    get_value(data, "borrower", borrower);
    if (borrower[0] == 0) strcpy(borrower, "unknown");

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    if (id[0] == 0) {
        printf("<p>Please enter the Book ID.</p>");
    }
    else {
        result = take_stock(id, name, &left);

        if (result == -1) {
            printf("<p>Error: could not open or save books.txt.</p>");
        }
        else if (result == 0) {
            printf("<p>No book found with this ID.</p>");
        }
        else if (result == -2) {
            printf("<p>Sorry, \"%s\" is out of stock right now.</p>", name);
        }
        else {
            /* today's date and the due date (today + LOAN_DAYS) */
            now = time(NULL);
            t = *localtime(&now);
            strftime(borrow_date, sizeof(borrow_date), "%Y-%m-%d", &t);

            t.tm_mday = t.tm_mday + LOAN_DAYS;
            t.tm_isdst = -1;
            due = mktime(&t);
            strftime(due_date, sizeof(due_date), "%Y-%m-%d", localtime(&due));

            /* save the borrow record */
            fp = fopen(BORROWED_FILE, "a");
            if (fp != NULL) {
                fprintf(fp, "%s | %s | %s | borrowed %s | due %s\n",
                        id, name, borrower, borrow_date, due_date);
                fclose(fp);
            }

            printf("<p>Book borrowed: %s (ID %s)</p>", name, id);
            printf("<p>Borrow date: %s</p>", borrow_date);
            printf("<p>Please return it by: <b>%s</b></p>", due_date);
            printf("<p>Late returns have a penalty of $14.</p>");
            printf("<p>Stock left: %d</p>", left);
        }
    }

    printf("<p><a href=\"/assignment/borrowbook.html\">Back</a></p>");
    printf("</body></html>");
    return 0;
}
