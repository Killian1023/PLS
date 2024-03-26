#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct {
    char OrderID[10];
    char StartDate[10];
    int OrderNUM;
    char ProductID[20];
} Order;

// Function declarations
void logInput(const char* input);
void updatePeriod(const char *startDate, const char *endDate);
void addPeriod(const char *startDate, const char *endDate);
void addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId);
void runPLS(const char *algorithm);
void printReport(const char *filename);
void addBatch(const char *filename);

int main() {
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
                updatePeriod(startDate, endDate);
            } else if (strcmp(command, "addORDER") == 0) {
                char *orderId = strtok(NULL, " ");
                char *orderDate = strtok(NULL, " ");
                int orderNum = atoi(strtok(NULL, " "));
                char *productId = strtok(NULL, "\n");
                addOrder(orderId, orderDate, orderNum, productId);
            } else if (strcmp(command, "runPLS") == 0) {
                char *algorithm = strtok(NULL, "\n");
                runPLS(algorithm);
            } else if (strcmp(command, "printREPORT") == 0) {
                char *filename = strtok(NULL, "\n");
                printReport(filename);
            } else if (strcmp(command, "addBATCH") == 0) {
                char *filename = strtok(NULL, "\n");
                addBatch(filename);
            } else {
                printf("Unknown command.\n");
            }
        }
    }
    return 0;
}


void logInput(const char* input) {
    const char* filename = "scheduler.log";
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s\n", input);
    fclose(file);
}

void updatePeriod(const char *startDate, const char *endDate) {
    const char* filename = "scheduler.log";
    FILE* file = fopen(filename, "r+");
    if (file == NULL) {
        file = fopen(filename, "w");
        if (file == NULL) {
            perror("Failed to open log file");
            return;
        }
        fprintf(file, "start_date=%s\nend_date=%s\n", startDate, endDate);
        fclose(file);
        return;
    }

    char buffer[8192]; // Adjust the buffer size based on expected file size
    size_t len = 0;
    int lineCounter = 0;
    while (fgets(buffer + len, sizeof(buffer) - len, file) && len < sizeof(buffer)) {
        if (++lineCounter > 2) { // Skip the first two lines (old period)
            len += strlen(buffer + len);
        }
    }

    // Rewind the file to start to overwrite
    rewind(file);

    // Write the new period information
    fprintf(file, "start_date=%s\nend_date=%s\n", startDate, endDate);

    // Write back the rest of the content
    fwrite(buffer, 1, len, file);

    // Truncate the file in case the new content is shorter than the old content
    fflush(file);
    off_t currentPos = ftello(file);
    if (currentPos != -1) {
        ftruncate(fileno(file), currentPos);
    }

    fclose(file);
}


// void addPeriod(const char *startDate, const char *endDate) {
//     printf("Adding period from %s to %s\n", startDate, endDate);
//     // The period updating functionality is assumed to be handled within logInput for this example
//     char logEntry[100];
//     snprintf(logEntry, sizeof(logEntry), "start_date=%s\nend_date=%s\n", startDate, endDate);
//     // This directly logs the period; in a real application, you might adjust to ensure the period is updated correctly
//     logInput(logEntry);
// }

void addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId) {
    printf("Adding order %s, Date: %s, Number: %d, Product: %s\n", orderId, orderDate, orderNum, productId);
    char input[200];
    snprintf(input, sizeof(input), 
             "[ORDER]\nOrderID=%s\nStartDate=%s\nOrderNUM=%d\nProductID=%s\n", 
             orderId, orderDate, orderNum, productId);
    const char* filename = "scheduler.log";
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        perror("Failed to open log file");
        return;
    }
    fprintf(file, "%s\n", input);
    fclose(file);
}

void runPLS(const char *algorithm) {
    printf("Running PLS with %s algorithm\n", algorithm);
    // Implementation of the scheduling logic would go here
}

void printReport(const char *filename) {
    printf("Printing report to %s\n", filename);
    // The functionality to generate and print a report would be implemented here
}

void addBatch(const char *filename) {
    printf("Adding batch from %s\n", filename);
    //read the dat file and call addOrder for each line
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open batch file");
        return;
    }
    // Read each line from the file
    char line[200];
    while (fgets(line, sizeof(line), file)) {
        // Extract the order details from the line
        char *orderId = strtok(line, " ");
        char *orderDate = strtok(NULL, " ");
        int orderNum = atoi(strtok(NULL, " "));
        char *productId = strtok(NULL, "\n");
        // Add the order
        addOrder(orderId, orderDate, orderNum, productId);
    }
    fclose(file);
}