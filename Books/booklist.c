#include <stdio.h>
int main() {
    FILE *fp;
    FILE *output;

    char line[200];
//opening the file in read mode for scaning
    fp = fopen("books.txt", "r");

    if (fp == NULL) {
        printf("File not found!");
        return 1;
    }
//creating a new file using write mode
    output = fopen("books.html", "w");

    if (output == NULL) {
        printf("Cannot create output file!");
        fclose(fp);
        return 1;
    }

    fprintf(output, "<h2>Library Book Records</h2>");
    fprintf(output, "<table border='1'>");

    fprintf(output,
        "<tr><th>ID</th><th>Book</th>"
        "<th>Author</th><th>Stock</th></tr>");

    while (fgets(line, sizeof(line), fp)) {

        int id, stock;
        char book[80], author[80];

        if (sscanf(line, "%d,%79[^,],%79[^,],%d",
                   &id, book, author, &stock) == 4) {

            fprintf(output,
                "<tr><td>%d</td><td>%s</td>"
                "<td>%s</td><td>%d</td></tr>",
                id, book, author, stock);
        }
    }

    fprintf(output, "</table>");

    fclose(fp);
    fclose(output);

    printf("Data successfully displayed in books.html");

    return 0;
}
