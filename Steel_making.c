#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    char OrderID[10];
    char StartDate[10];
    int OrderNUM;
    char ProductID[20];
} Order;

// Corrected function declarations
void addPeriod(const char *startDate, const char *endDate);
void addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId);
void runPLS(const char *algorithm);
void printReport(const char *filename);
void addBatch(const char *filename);

int main(){
    while(1) {
        char input[200];
        printf("Please enter:\n> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Error reading input.\n");
            continue;
        }
        if (strncmp(input, "exitPLS", 7) == 0) {
            printf("Bye-bye!\n");
            break;
        }

        char *command = strtok(input, " ");

        if (command != NULL) {
            if (strcmp(command, "addPERIOD") == 0) {
                char *startDate = strtok(NULL, " ");
                char *endDate = strtok(NULL, "\n");
                addPeriod(startDate, endDate); 
            } else if (strcmp(command, "addORDER") == 0) { //addORDER P0001 2024-06-10 2000 Product_A
                char *orderId = strtok(NULL, " ");
                char *orderDate = strtok(NULL, " ");
                int orderNum = atoi(strtok(NULL, " "));
                char *productId = strtok(NULL, "\n"); 
                addOrder(orderId, orderDate, orderNum, productId); 
            } else if (strcmp(command, "runPLS") == 0) {
                // Extract arguments and call runPLS
                for(int i = 0;i<100;i++){
                    printf("Running PLS with %d algorithm\n", i);
                }
            } else if (strcmp(command, "printREPORT") == 0) {
                // This should be handled in conjunction with runPLS due to the pipe (|)
                for(int i = 0;i<100;i++){
                    printf("Printing report to %d\n", i);
                }
            } else if (strcmp(command, "addBATCH") == 0) {
                // Extract arguments and call addBatch
            } else {
                printf("Unknown command.\n");
            }
        }
    }

    return 0;
}

void logInput(const char* input) {
    const char* filename = "scheduler.log";
    FILE* file = fopen(filename, "a"); // Open for appending.
    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s\n", input); // Append the structured input.
    fclose(file); // Always close the file.
}


void addPeriod(const char *startDate, const char *endDate) {
    printf("Adding period from %s to %s\n", startDate, endDate);
    char logEntry[100]; // Ensure this is large enough to hold your log entry.
    snprintf(logEntry, sizeof(logEntry), "[PERIOD]\nstart_date=%s\nend_date=%s\n", startDate, endDate);
    logInput(logEntry);
}

void addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId) {
    printf("Adding order %s, Date: %s, Number: %d, Product: %s\n", orderId, orderDate, orderNum, productId);
    char logEntry[200]; // Adjust size based on potential length of the order details.
    snprintf(logEntry, sizeof(logEntry), 
             "[ORDER]\nOrderID=%s\nStartDate=%s\nOrderNUM=%d\nProductID=%s\n", 
             orderId, orderDate, orderNum, productId);
    logInput(logEntry);
}


void runPLS(const char *algorithm) {
    printf("Running PLS with %s algorithm\n", algorithm);
}

void printReport(const char *filename) {
    printf("Printing report to %s\n", filename);
}

void addBatch(const char *filename) {
    printf("Adding batch from %s\n", filename);
}
