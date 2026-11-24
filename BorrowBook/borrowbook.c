#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <time.h>    

#define BOOKS_FILE    "C:/xampp/htdocs/assignment/books.txt"
#define BORROWED_FILE "C:/xampp/htdocs/assignment/borrowed.txt"
#define LOAN_DAYS 14     
#define SEP '|'          //character that separates the fields in books.txt 
#define MAX_LINES 200    //books.txt can hold up to this many lines 

//books.txt line format:  name|id|author|stock 

char lines[MAX_LINES][500];   //ll lines of books.txt are kept here 

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
// remove spaces from end and start of word
char *trim(char *s) {
    char *end;
    while (*s == ' ') s++;           //ignore initial space 
    end = s + strlen(s);             //pointing to end of string

    while (end > s && (end[-1] == ' ' || end[-1] == '\n' || end[-1] == '\r')) {
        end--;
        *end = 0;
    }
    return s;
}

// finds the book in books.txt, takes 1 from its stock and saves the file
 //  returns  1 = done
           // 0 = book not found
         //  -1 = file problem
         //  -2 = no stock left 

int take_stock(char *id, char *name, int *left) {
    FILE *fp;
    int count = 0;                  //lines read from the file
    int i, n, stock, found = -1;     //found is indeex pointed for line
    char temp[500];                  //stores lines from txt file being used currently
    char *f[4];                      //pointer to field names
    char *p;

    fp = fopen(BOOKS_FILE, "r");
    if (fp == NULL) return -1;      
    while (count < MAX_LINES && fgets(lines[count], 500, fp)) count++;
    fclose(fp);

   //===================================================================================================================================
//finding book
    for (i = 0; i < count; i++) {
        strcpy(temp, lines[i]);      // copy so the original line stays safe for other purpose

        //split the line into 4 parts: name, id, author, stock 
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
        if (n < 4) continue;      

        /* remove extra spaces around each part */
        f[0] = trim(f[0]);
        f[1] = trim(f[1]);
        f[2] = trim(f[2]);
        f[3] = trim(f[3]);

        if (strcmp(f[1], id) == 0) {
            strcpy(name, f[0]);     
            stock = atoi(f[3]);     

            if (stock <= 0) return -2;   //shows no more stock available

            stock = stock - 1;      // the copy has been borrowed 
            *left = stock;         

            sprintf(lines[i], "%s%c%s%c%s%c%d\n",
                    f[0], SEP, f[1], SEP, f[2], SEP, stock);
            found = i;               
            break;                   
        }
    }

    if (found == -1) return 0;       //id not found

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
//extracting fields required for the process
    get_value(data, "bookid", id);
    get_value(data, "borrower", borrower);
    if (borrower[0] == 0) strcpy(borrower, "unknown");   /* default name if left empty */
//output in terminal
    //=================================================================================================================================
    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>");

    if (id[0] == 0) {// Presence Check
        printf("<p>Please enter the Book ID.</p>");
    }
    else {
    //setting default stock as 1
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
           //Extra feature for current time and date from html to c variable
            now = time(NULL);               
            t = *localtime(&now);            
            strftime(borrow_date, sizeof(borrow_date), "%Y-%m-%d", &t);  

            t.tm_mday = t.tm_mday + LOAN_DAYS;   
            t.tm_isdst = -1;                    
            due = mktime(&t);                    
            strftime(due_date, sizeof(due_date), "%Y-%m-%d", localtime(&due));   

           //================================================================================================
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
