#include <stdio.h>   
#include <string.h> 

#define BOOKS_FILE "C:/xampp/htdocs/assignment/books.txt"
#define HTML_FILE  "C:/xampp/htdocs/assignment/books/books.html"
//removing extra spaces
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
//======================================================
int main() {
    FILE *fp; //dealing with input file
    FILE *output;        //dealing with output file(new one)
    char line[300], copy[300];/* 'line' is chopped up by strtok, 'copy' keeps an
                                 untouched version for error messages */
    char *book, *id, *author, *stock; 
    int total = 0, shown = 0; /* total = non-empty lines read,
                                 shown = lines successfully written to HTML */

    
    fp = fopen(BOOKS_FILE, "r");
    if (fp == NULL) {
        printf("File not found: %s\n", BOOKS_FILE);
        return 1;
    }
    output = fopen(HTML_FILE, "w");
    if (output == NULL) {
        printf("Cannot create output file: %s\n", HTML_FILE);
        fclose(fp);          
        return 1;
    }
//writing for html page output
    fprintf(output, "<html><body>");
    fprintf(output, "<h2>Library Book Records</h2>");
    fprintf(output, "<table border='1' cellpadding='6'>");
    fprintf(output, "<tr><th>ID</th><th>Book</th><th>Author</th><th>Stock</th></tr>");

    /* Read the input file one line at a time until the end of the file */
    while (fgets(line, sizeof(line), fp)) {
        /* Save a copy of the line, because strtok will modify 'line' */
        strcpy(copy, line);
        /* Remove the newline characters from the copy (used in error output) */
        copy[strcspn(copy, "\r\n")] = 0;
        if (copy[0] == 0) continue;           
        total++;                          
//separating the lines in txt file for processing
        book   = strtok(line, "|,");
        id     = strtok(NULL, "|,");
        author = strtok(NULL, "|,");
        stock  = strtok(NULL, "|,");
//only work if every data required is added
        if (book && id && author && stock) {
        // removing unwanted spaces around each data 
            fprintf(output, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                    trim(id), trim(book), trim(author), trim(stock));
            shown++;                         //counting complete row traversal in txt file
        } else {
            // when any one data is not present 
            printf("Could not read line %d: [%s]\n", total, copy);
        }
    }

    fprintf(output, "</table>");
   
    if (shown == 0) fprintf(output, "<p>No books found.</p>");

    fprintf(output, "</body></html>");
    fclose(fp);
    fclose(output);

   // summary for console
    printf("Lines read: %d, books shown: %d\n", total, shown);
    return 0;   
}
    fprintf(output, "<html><body>");
    fprintf(output, "<h2>Library Book Records</h2>");
    fprintf(output, "<table border='1' cellpadding='6'>");
    fprintf(output, "<tr><th>ID</th><th>Book</th><th>Author</th><th>Stock</th></tr>");

    while (fgets(line, sizeof(line), fp)) {
        strcpy(copy, line);
        copy[strcspn(copy, "\r\n")] = 0;
        if (copy[0] == 0) continue;            //skipping empty lines
        total++;

        book   = strtok(line, "|,");
        id     = strtok(NULL, "|,");
        author = strtok(NULL, "|,");
        stock  = strtok(NULL, "|,");

        if (book && id && author && stock) {
            fprintf(output, "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                    trim(id), trim(book), trim(author), trim(stock));
            shown++;
        } else {
            printf("Could not read line %d: [%s]\n", total, copy);
        }
    }

    fprintf(output, "</table>");
    if (shown == 0) fprintf(output, "<p>No books found.</p>");
    fprintf(output, "</body></html>");

    fclose(fp);
    fclose(output);

    printf("Lines read: %d, books shown: %d\n", total, shown);
    return 0;
}
