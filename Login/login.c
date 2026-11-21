#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward slashes avoid the "\x", "\h", "\a" escape-sequence problem */
#define OUT_FILE "C:/xampp/htdocs/assignment/login.txt"

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

/* Print text safely inside HTML */
static void print_escaped(const char *s) {
    for (; *s; s++) {
        if (*s == '<') printf("&lt;");
        else if (*s == '>') printf("&gt;");
        else if (*s == '&') printf("&amp;");
        else if (*s == '"') printf("&quot;");
        else putchar(*s);
    }
}

/* Read login.txt: returns 1 if the username exists, and copies its password */
static int find_user(const char *user, char *pass_out, size_t sz) {
    FILE *f = fopen(OUT_FILE, "r");
    if (f == NULL) return 0;

    char line[1024];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *sep = strchr(line, '|');
        if (sep == NULL) continue;
        *sep = 0;
        if (strcmp(line, user) == 0) {
            strncpy(pass_out, sep + 1, sz - 1);
            pass_out[sz - 1] = 0;
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

    char action[32], username[128], password[128];
    get_field(data, "action", action, sizeof action);
    get_field(data, "username", username, sizeof username);
    get_field(data, "password", password, sizeof password);

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    if (username[0] == 0 || password[0] == 0) {
        printf("<p>Please enter both username and password.</p>");
    }
    else if (strcmp(action, "register") == 0) {
        char stored[128];
        if (find_user(username, stored, sizeof stored)) {
            printf("<p>That username is already taken.</p>");
        } else {
            FILE *fptr = fopen(OUT_FILE, "a");      /* write to login.txt */
            if (fptr == NULL) {
                printf("<p>Error: could not open " OUT_FILE "</p>");
            } else {
                fprintf(fptr, "%s|%s\n", username, password);
                fclose(fptr);
                printf("<p>Account created for ");
                print_escaped(username);
                printf(". You can now <a href=\"/assignment/login.html\">log in</a>.</p>");
            }
        }
    }
    else if (strcmp(action, "login") == 0) {
        char stored[128];
        if (find_user(username, stored, sizeof stored) && strcmp(stored, password) == 0) {
            printf("<p>Login successful. Welcome, ");
            print_escaped(username);
            printf("!</p>");
        } else {
            printf("<p>Login failed: wrong username or password.</p>");
        }
    }
    else {
        printf("<p>Unknown action.</p>");
    }

    printf("<p><a href=\"/assignment/login.html\">Login</a> | "
           "<a href=\"/assignment/register.html\">Register</a></p>");
    printf("</body></html>");
    return 0;
}
