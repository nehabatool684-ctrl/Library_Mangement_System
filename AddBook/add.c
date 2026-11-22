#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward slashes avoid the "\x", "\h", "\a" escape-sequence problem */
#define OUT_FILE "C:/xampp/htdocs/assignment/books.txt"

/*
 * url_decode: converts URL-encoded text back to normal text.
 * Browsers encode form data, so "Harry+Potter%21" arrives instead of "Harry Potter!".
 * dst receives the decoded text, src is the encoded input.
 */
static void url_decode(char *dst, const char *src) {
    while (*src) {
        /* '+' means a space in URL encoding */
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        /* '%XX' is a hex-encoded byte: read the two hex digits, convert
           them to a character, and skip all three input characters */
        else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = { src[1], src[2], 0 };
            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        }
        /* any other character is copied unchanged */
        else {
            *dst++ = *src++;
        }
    }
    /* terminate the decoded string */
    *dst = 0;
}

/*
 * get_field: finds one field's value inside form data such as
 * "bookname=Dune&bookid=12&author=Herbert", decodes it, and stores it in out.
 * If the field is missing, out is left as an empty string.
 */
static void get_field(const char *data, const char *name, char *out, size_t outsz) {
    /* build the search key, e.g. "bookname=", and start with an empty result */
    char key[64];
    snprintf(key, sizeof key, "%s=", name);
    size_t klen = strlen(key);
    const char *p = data;
    out[0] = 0;

    /* look for each occurrence of the key in the data */
    while ((p = strstr(p, key)) != NULL) {
        /* accept the match only if it is at the start or right after '&',
           so "author=" is not matched inside a longer name like "coauthor=" */
        if (p == data || p[-1] == '&') {
            /* measure the value: everything up to the next '&' or the end */
            p += klen;
            size_t n = strcspn(p, "&");

            /* copy the raw value into a temporary buffer and decode it */
            char *raw = malloc(n + 1);
            char *dec = malloc(n + 1);
            memcpy(raw, p, n);
            raw[n] = 0;
            url_decode(dec, raw);

            /* copy the result into the caller's buffer safely, then free memory */
            strncpy(out, dec, outsz - 1);
            out[outsz - 1] = 0;
            free(raw);
            free(dec);

            /* keep each record on one line */
            for (char *c = out; *c; c++)
                if (*c == '\n' || *c == '\r') *c = ' ';
            return;
        }
        /* not a real field start: skip past this match and keep searching */
        p += klen;
    }
}

/*
 * main: the CGI entry point. Reads the submitted form data, saves one record
 * to the text file, and prints an HTML confirmation page.
 */
int main(void) {
    char data[4096] = "";
    char *method = getenv("REQUEST_METHOD");

    /* POST: the form data arrives on standard input; CONTENT_LENGTH says how
       many bytes to read (capped to the buffer size to prevent overflow) */
    if (method && strcmp(method, "POST") == 0) {
        char *len = getenv("CONTENT_LENGTH");
        int n = len ? atoi(len) : 0;
        if (n > (int)sizeof(data) - 1) n = sizeof(data) - 1;
        size_t got = fread(data, 1, n, stdin);
        data[got] = 0;
    }
    /* GET: the form data is in the URL after '?', supplied as QUERY_STRING */
    else {
        char *q = getenv("QUERY_STRING");
        if (q) strncpy(data, q, sizeof(data) - 1);
    }
//=====================OUR CONCERN=================================
    /* extract and decode the three form fields */
    char name[256], id[64], author[256];
    get_field(data, "bookname", name, sizeof name);
    get_field(data, "bookid", id, sizeof id);
    get_field(data, "author", author, sizeof author);

    /* CGI response header (ends with a blank line), then the start of the page */
    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    /* open the file in append mode so earlier records are kept */
    FILE *fptr = fopen(OUT_FILE, "a");

    /* if the file could not be opened, show an error message */
    if (fptr == NULL) {
        printf("<p>Error: could not open " OUT_FILE "</p>");
        printf("<p>Check that the folder exists.</p>");
    }
    /* otherwise save the record as "id | name | author", close the file,
       and show the saved values with a success message */
    else {
        fprintf(fptr, "%s | %s | %s\n", id, name, author);
        fclose(fptr);
        printf("<p>Book Name: %s</p><p>Book ID: %s</p><p>Author: %s</p>", name, id, author);
        printf("<p>Added successfully.</p>");
    }

    /* link back to the form, close the HTML page, and finish */
    printf("<p><a href=\"/assignment/form.html\">Back to form</a></p>");
    printf("</body></html>");
    return 0;
}
