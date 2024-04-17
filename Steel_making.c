#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#define _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200809L
#define MAX_ORDERS 1000
#define ORDER_ID_LENGTH 20
const int CAPACITY_X = 300; // indicate the capacity of plantx
const int CAPACITY_Y = 400; // indicate the capacity of planty
const int CAPACITY_Z = 500; // indicate the capacity of plantz
const int NUM_PLANT = 3;    // indicate the number of plant

typedef struct Order
{
    struct Order *next;
    char Order_Number[10];
    char Due_Date[11];
    int Quantity;
    char Product_Name[20];
} Order;
Order *head = NULL;

typedef struct
{
    char start_date[11];
    char end_date[11];
} Period;
Period *pointertoperiod = NULL;

typedef struct
{
    char date[11];
    char productId[20];
    char orderId[10];
    int quantity;
    char plantId[10];
} Schedule;

typedef struct
{
    char orderID[10];
    char start[11];
    char end[11];
    int days;
    int quantity;
    char plantID[10];
} Ack_Detail;

typedef struct
{
    char orderID[10];
    char productID[20];
    char date[11];
    int quantity;
} Rej_Detail;

// Function declarations
void addPeriod(const char *startDate, const char *endDate);
int addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId);
void runPLS(const char *algorithm);
void addBatch(const char *filename);
void runFCFS(Order *head, char startDate[11], char endDate[11], const char *filename, bool FCFSorSRT);
void runSRT(Order *head, char startDate[11], char endDate[11], const char *filename);
Order *insert(Order *curr, Order *node);
Order *sortList(Order *head);
void Oracle(Order *head, char startDate[11], char endDate[11], const char *filename);
void printReport(const char *input, const char *filename);

int main()
{

    printf("          ~~WELCOME TO PLS~~\n");
    printf("current avilible scheduler: 1.FCFS, 2.SRT, 3.Oracle\n");

    while (1)
    {
        char input[200];
        printf("Please enter:\n> ");
        if (!fgets(input, sizeof(input), stdin))
        {
            printf("Error reading input.\n");
            continue;
        }
        else if (strncmp(input, "exitPLS", 7) == 0)
        {
            printf("Bye-bye!\n");
            break;
        }
        else
        {
            char *command = strtok(input, " ");
            if (command != NULL)
            {
                if (strcmp(command, "addPERIOD") == 0)
                {
                    char *startDate = strtok(NULL, " ");
                    char *endDate = strtok(NULL, "\n");

                    struct tm start = {0}, end = {0};
                    strptime(startDate, "%Y-%m-%d", &start);
                    strptime(endDate, "%Y-%m-%d", &end);
                    time_t start_time = mktime(&start);
                    time_t end_time = mktime(&end);
                    int days = difftime(end_time, start_time) / (86400) + 1;
                    if (days < 0)
                    {
                        printf("The period you input doesn't exist, please try another one.\n");
                        continue;
                    }
                    Period *period = (Period *)malloc(sizeof(Period));
                    strncpy(period->start_date, startDate, 11);
                    period->start_date[10] = '\0';
                    strncpy(period->end_date, endDate, 11);
                    period->end_date[10] = '\0';
                    printf("Successfully added period starting from %s to %s.\n", period->start_date, period->end_date);
                    pointertoperiod = period;
                }

                else if (strcmp(command, "addORDER") == 0)
                {
                    char *orderId = strtok(NULL, " ");
                    char *orderDate = strtok(NULL, " ");
                    int orderNum = atoi(strtok(NULL, " "));
                    char *productId = strtok(NULL, "\n");
                    addOrder(orderId, orderDate, orderNum, productId);
                }

                else if (strcmp(command, "runPLS") == 0)
                {
                    char *algorithm = strtok(NULL, " \n");
                    char *PIPE_SYMBOL = strtok(NULL, " ");
                    char *reportCommand = strtok(NULL, " >");
                    char *skip = strtok(NULL, " ");
                    char *filename = strtok(NULL, "\n");

                    if (algorithm == NULL)
                    {
                        printf("No algorithm provided.\n");
                    }
                    else
                    {
                        int P2C[2];
                        int C2P[2];
                        if (pipe(P2C) < 0)
                        {
                            printf("Pipe from parent to child creation error\n");
                            exit(1);
                        }
                        if (pipe(C2P) < 0)
                        {
                            printf("Pipe from child to parent creation error\n");
                            exit(1);
                        }

                        pid_t pid = fork();
                        if (pid == 0)
                        {

                            while (1)
                            {

                                close(P2C[1]); // close the write end for the p2c pipe
                                close(C2P[0]); // close the read end for the c2p pipe

                                // Read from the pipe
                                int size;
                                char buffer[50];
                                size = read(P2C[0], buffer, sizeof(buffer));

                                if (buffer[0] == 'Y')
                                { // means the parent allow the child to start the scheduling
                                    if (strcmp(algorithm, "FCFS") == 0)
                                    {
                                        runFCFS(head, pointertoperiod->start_date, pointertoperiod->end_date, filename, true);
                                    }
                                    else if (strcmp(algorithm, "SRT") == 0)
                                    {
                                        runSRT(head, pointertoperiod->start_date, pointertoperiod->end_date, filename);
                                    }
                                    else if (strcmp(algorithm, "Oracle") == 0)
                                    {
                                        Oracle(head, pointertoperiod->start_date, pointertoperiod->end_date, filename);
                                    }
                                    else
                                    {
                                        printf("Unsupported algorithm.\n");
                                    }
                                    if (PIPE_SYMBOL != NULL && strcmp(PIPE_SYMBOL, "|") == 0 && reportCommand != NULL && strcmp(reportCommand, "printREPORT") == 0 && filename != NULL)
                                    {
                                        FILE *file = freopen(filename, "w", stdout);
                                        if (file)
                                        {
                                            char input_file[256];
                                            sprintf(input_file, "%s_tmp.txt", filename); // create the temp name waiting for being passed to printReport
                                            printReport(input_file, filename);
                                            fclose(file);
                                            remove(input_file); // remove the temp file
                                        }
                                        else
                                        {
                                            perror("Failed to open file");
                                        }
                                    }
                                }
                                exit(0);
                            }
                        }
                        // parent process
                        close(P2C[0]);                                // close the read end for p2c pipes.
                        close(C2P[1]);                                // close the write end for c2p pipes.
                        char msg[] = {"Yes, start the algorithm"}; // means the child received this message will start the game as the first player.
                        write(P2C[1], msg, sizeof(msg));
                        waitpid(pid, NULL, 0);
                    }
                }
                else if (strcmp(command, "addBATCH") == 0)
                {
                    char *filename = strtok(NULL, "\n");
                    addBatch(filename);
                }
                else
                {
                    printf("Unknown command.\n");
                }
            }
        }
    }
    return 0;
}

void runFCFS(Order *head, char startDate[11], char endDate[11], const char *filename, bool FCFSorSRT)
{

    // store the rejected orders

    Order skippedOrders[MAX_ORDERS];
    int skippedCount = 0; // num of orders get rejected
    int numOrder = 1;
    int NUM_Scheules = 0;

    struct tm start = {0}, end = {0};
    strptime(startDate, "%Y-%m-%d", &start);
    strptime(endDate, "%Y-%m-%d", &end);
    time_t start_time = mktime(&start);
    time_t end_time = mktime(&end);
    int days = difftime(end_time, start_time) / (86400) + 1;

    Schedule schedules[days][3]; // store the schedule result
    strptime(startDate, "%Y-%m-%d", &start);

    for (int i = 0; i < days; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            strftime(schedules[i][j].date, sizeof(schedules[i][j].date), "%Y-%m-%d", &start);
            strcpy(schedules[i][j].productId, "NA");
            strcpy(schedules[i][j].orderId, "NA");
            schedules[i][j].quantity = 0;
            sprintf(schedules[i][j].plantId, "Plant_%c", 'X' + j);
        }
        start.tm_mday++;
        mktime(&start);
    }

    strptime(startDate, "%Y-%m-%d", &start);
    Order *current = head;
    if (current == NULL)
    {
        printf("The list is empty.\n");
        return;
    }
    int dayIndex = 0;
    int remainingQuantity = current->Quantity;
    while (current != NULL && dayIndex < days)
    {

        // check the due date.
        struct tm due = {0};
        time_t current_time = mktime(&start);
        int allocated = 0;
        bool not_use_x = false;
        bool not_use_y = false;
        bool plant_used[3] = {false, false, false};
        bool switchedOrder = false;
        int production_plant[3] = {300, 400, 500};
        int max_production_plant_index = 2;
        int test = dayIndex + 1;
        for (int x = 0; x < 3; x++)
        {
            strptime(current->Due_Date, "%Y-%m-%d", &due);
            time_t due_time = mktime(&due);
            int remainDays = difftime(due_time, current_time) / (86400) + 1;

            if (difftime(current_time, due_time) > 0 || (remainingQuantity / remainDays) > remainingQuantity - allocated)
            {
                if (skippedCount < MAX_ORDERS)
                {
                    memcpy(&skippedOrders[skippedCount], current, sizeof(Order));
                    skippedCount++;
                }
                current = current->next;
                if (current == NULL)
                {
                    break;
                }
                numOrder++;
                continue;
            }

            if (remainingQuantity > production_plant[max_production_plant_index])
            {
                for (int j = 0; j < 3; j++)
                {
                    not_use_x = false;
                    not_use_y = false;
                    if ((max_production_plant_index == 2 && (remainingQuantity > 800 && remainingQuantity <= 900)) && !plant_used[0])
                    {
                        not_use_x = true;
                    }
                    if ((max_production_plant_index == 2 && (remainingQuantity > 700 && remainingQuantity <= 800)) && !plant_used[1])
                    {
                        not_use_y = true;
                    }
                    if (not_use_x && j == 0)
                        continue;
                    if (not_use_y && j == 1)
                        continue;
                    if (!plant_used[j])
                    {
                        strcpy(schedules[dayIndex][j].productId, current->Product_Name);
                        strcpy(schedules[dayIndex][j].orderId, current->Order_Number);
                        sprintf(schedules[dayIndex][j].plantId, "Plant_%c", 'X' + j);
                        plant_used[j] = true;
                        if (j == max_production_plant_index)
                        {
                            for (int i = 0; i < 3; i++)
                            {
                                if (!plant_used[i])
                                    max_production_plant_index = i;
                            }
                        }
                        if (remainingQuantity - allocated > production_plant[j])
                        {
                            schedules[dayIndex][j].quantity = production_plant[j];
                            allocated += schedules[dayIndex][j].quantity;
                        }
                        else
                        {
                            schedules[dayIndex][j].quantity = remainingQuantity - allocated;
                            allocated += schedules[dayIndex][j].quantity;
                        }
                        break;
                    }
                }
            }
            else
            {
                for (int i = 0; i < 3; i++)
                {
                    if (!plant_used[i])
                    {
                        if (remainingQuantity - allocated > production_plant[i])
                        {
                            continue;
                        }
                        else
                        {
                            strcpy(schedules[dayIndex][i].productId, current->Product_Name);
                            strcpy(schedules[dayIndex][i].orderId, current->Order_Number);
                            sprintf(schedules[dayIndex][i].plantId, "Plant_%c", 'X' + i);
                            plant_used[i] = true;
                            if (i == max_production_plant_index)
                            {
                                for (int k = 0; k < 3; k++)
                                {
                                    if (!plant_used[k])
                                        max_production_plant_index = k;
                                }
                            }
                            schedules[dayIndex][i].quantity = remainingQuantity - allocated;
                            allocated += schedules[dayIndex][i].quantity;
                            break;
                        }
                    }
                }
            }
            NUM_Scheules++;
            if (remainingQuantity <= allocated)
            {
                current = current->next;
                if (current == NULL)
                {
                    break;
                }
                numOrder++;
                allocated = 0;
                remainingQuantity = current->Quantity;
                switchedOrder = true;
            }
        }

        if (remainingQuantity != allocated)
        {
            remainingQuantity -= allocated;
            dayIndex++;
            start.tm_mday++;
            mktime(&start);
        }
        else
        {
            if (current == NULL)
                break;
            current = current->next;
            if (current == NULL)
                break;
            numOrder++;
            remainingQuantity = current->Quantity;
            dayIndex++;
            start.tm_mday++;
            mktime(&start);
        }
    }
    if (current != NULL)
    {
        while (current != NULL)
        {
            memcpy(&skippedOrders[skippedCount], current, sizeof(Order));
            skippedCount++;
            current = current->next;
            numOrder++;
        }
    }

    char tmp_filename[256];
    sprintf(tmp_filename, "%s_tmp.txt", filename);
    FILE *file = fopen(tmp_filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    if (FCFSorSRT)
    {
        fprintf(file, "***Production Schedule: FCFS***\n");
    }
    else
    {
        fprintf(file, "***Production Schedule: SRT***\n");
    }
    fprintf(file, "***Accepted Order: %d, Accepted Sch: %d, Rejected Order: %d, Total Days: %d***\n", numOrder - skippedCount, NUM_Scheules, skippedCount, days);
    fprintf(file, "***Accepted Orders***\n");
    for (int i = 0; i <= dayIndex; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (schedules[i][j].quantity > 0)
            {
                fprintf(file, "%s %s %s %d %s\n",
                        schedules[i][j].date, schedules[i][j].productId, schedules[i][j].orderId,
                        schedules[i][j].quantity, schedules[i][j].plantId);
            }
        }
    }
    fprintf(file, "***Rejected Orders***\n");
    for (int i = 0; i < skippedCount; i++)
    {
        fprintf(file, "%s %s %s %d\n",
                skippedOrders[i].Due_Date, skippedOrders[i].Product_Name, skippedOrders[i].Order_Number,
                skippedOrders[i].Quantity);
    }
    fclose(file);
}

int *Allocate_Plants(int aviliblePlant[NUM_PLANT], int remainingQuantity)
{

    int *Allocate_Plants;
    int *array = (int *)malloc(NUM_PLANT * sizeof(int));
    int i, j, k;
    int numofx, numofy, numofz;
    int min_overhead = 500;
    int check = 0; // 0 indicate no result

    for (int i = 0; i < 3; i++)
    {
        array[i] = -1;
    }

    for (i = 0; i <= aviliblePlant[0]; i++)
    {
        int quantity = 0;
        quantity += i * CAPACITY_X;
        if (quantity < remainingQuantity)
        {
            int maxnumofY = ((remainingQuantity - quantity) / CAPACITY_Y) + 1;
            for (j = 0; j <= maxnumofY && j <= aviliblePlant[1]; j++)
            {
                if ((quantity + j * CAPACITY_Y) >= remainingQuantity)
                {
                    if ((quantity + j * CAPACITY_Y - remainingQuantity) < min_overhead)
                    {
                        min_overhead = quantity + j * CAPACITY_Y - remainingQuantity;
                        numofx = i;
                        numofy = j;
                        numofz = 0;
                        check = 1;
                    }
                }
                else
                {
                    int maxnumofz = ((remainingQuantity - quantity - j * CAPACITY_Y) / CAPACITY_Z) + 1;
                    if ((quantity + j * CAPACITY_Y + maxnumofz * CAPACITY_Z - remainingQuantity) < min_overhead && maxnumofz <= aviliblePlant[2])
                    {
                        min_overhead = quantity + j * CAPACITY_Y + maxnumofz * CAPACITY_Z - remainingQuantity;
                        numofx = i;
                        numofy = j;
                        numofz = maxnumofz;
                        check = 1;
                    }
                }
            }
        }
        else
        {
            if ((quantity - remainingQuantity) < min_overhead)
            {
                min_overhead = quantity - remainingQuantity;
                numofx = i;
                numofy = 0;
                numofz = 0;
                check = 1;
            }
        }
    }
    if (check == 1)
    {
        array[0] = numofx;
        array[1] = numofy;
        array[2] = numofz;
    }

    return array;
}

void Oracle(Order *head, char startDate[11], char endDate[11], const char *filename)
{
    struct tm start = {0}, end = {0};
    strptime(startDate, "%Y-%m-%d", &start);
    strptime(endDate, "%Y-%m-%d", &end);
    time_t start_time = mktime(&start);
    time_t end_time = mktime(&end);
    int NUM_DAY = difftime(end_time, start_time) / (86400) + 1;

    Schedule schedules[NUM_DAY][NUM_PLANT];
    Schedule rejected[MAX_ORDERS];
    int NUM_ORDER = 0;
    int NUM_Scheules = 0;
    int numofrejection = 0;

    strptime(startDate, "%Y-%m-%d", &start);
    for (int i = 0; i < NUM_DAY; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            strftime(schedules[i][j].date, sizeof(schedules[i][j].date), "%Y-%m-%d", &start);
            strcpy(schedules[i][j].productId, "NA");
            strcpy(schedules[i][j].orderId, "NA");
            schedules[i][j].quantity = 0;
            sprintf(schedules[i][j].plantId, "Plant_%c", 'X' + j);
        }
        start.tm_mday++;
        mktime(&start);
    }
    strptime(startDate, "%Y-%m-%d", &start);

    // here will call the sorting algorithm and work on the sorted array //it may be due date based sorting or quantity based sorting or even no soritng

    Order *current = head;
    int remainingQuantity = current->Quantity; // indiate the current order's remaining quantity.
    int max_production_plant_index = 2;        // this index indicate which aviliable plant has the max producing ability
    // int SCHEDULES[NUM_PLANT][NUM_DAY][2];      // store the orderid and the quantity
    int avilibleNumOfDay[NUM_PLANT]; // indicate the num of days that each of the three plants are aviliable
    int nextavilibleDay[NUM_PLANT];
    for (int i = 0; i < 3; i++)
    {
        avilibleNumOfDay[i] = NUM_DAY; // indicate current all the plants for all the days are aviliable
    }
    for (int i = 0; i < 3; i++)
    {
        nextavilibleDay[i] = 0; // indicate current all the plants for all the days are aviliable
    }

    while (current != NULL) // go through all the orders
    {
        int quantity = current->Quantity;
        int *result;
        NUM_ORDER++;

        struct tm due = {0};
        strptime(current->Due_Date, "%Y-%m-%d", &due);
        time_t due_time = mktime(&due);
        int remaindays = difftime(due_time, start_time) / (86400) + 1;

        int avilibiltyBeforeDuedate[NUM_PLANT];
        for (int i = 0; i < 3; i++)
        {
            avilibiltyBeforeDuedate[i] = remaindays - nextavilibleDay[i];
        }
        result = Allocate_Plants(avilibiltyBeforeDuedate, quantity);

        if (result[0] == -1)
        {
            // printf("this order has been rejected\n");
            strftime(rejected[numofrejection].date, sizeof(rejected[numofrejection].date), "%Y-%m-%d", &start);
            strcpy(rejected[numofrejection].productId, current->Product_Name);
            strcpy(rejected[numofrejection].orderId, current->Order_Number);
            rejected[numofrejection].quantity = current->Quantity;
            numofrejection++;
            current = current->next;
        }
        else
        {
            int numofx = result[0];
            int numofy = result[1];
            int numofz = result[2];
            int lefthole = (CAPACITY_X * numofx + CAPACITY_Y * numofy + CAPACITY_Z * numofz) - quantity;
            for (int i = 0; i < numofx; i++)
            {
                strcpy(schedules[nextavilibleDay[0]][0].productId, current->Product_Name);
                strcpy(schedules[nextavilibleDay[0]][0].orderId, current->Order_Number);
                schedules[nextavilibleDay[0]][0].quantity = CAPACITY_X;
                avilibleNumOfDay[0]--;
                nextavilibleDay[0]++;
            }
            for (int i = 0; i < numofy; i++)
            {
                strcpy(schedules[nextavilibleDay[1]][1].productId, current->Product_Name);
                strcpy(schedules[nextavilibleDay[1]][1].orderId, current->Order_Number);
                schedules[nextavilibleDay[1]][1].quantity = CAPACITY_Y;
                avilibleNumOfDay[1]--;
                nextavilibleDay[1]++;
            }
            for (int i = 0; i < numofz; i++)
            {
                strcpy(schedules[nextavilibleDay[2]][2].productId, current->Product_Name);
                strcpy(schedules[nextavilibleDay[2]][2].orderId, current->Order_Number);
                schedules[nextavilibleDay[2]][2].quantity = CAPACITY_Z;
                avilibleNumOfDay[2]--;
                nextavilibleDay[2]++;
            }
            if (numofz != 0)
            {
                schedules[nextavilibleDay[2] - 1][2].quantity -= lefthole;
            }
            else if (numofy != 0)
            {
                schedules[nextavilibleDay[1] - 1][1].quantity -= lefthole;
            }
            else
            {
                schedules[nextavilibleDay[0] - 1][0].quantity -= lefthole;
            }
            NUM_Scheules += (numofx + numofy + numofz);
            current = current->next;
        }
    }
    char tmp_filename[256];
    sprintf(tmp_filename, "%s_tmp.txt", filename);
    FILE *file = fopen(tmp_filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }
    fprintf(file, "***Production Schedule: Oracle***\n");
    fprintf(file, "***Accepted Order: %d, Accepted Sch: %d, Rejected Order: %d, Total Days: %d***\n", NUM_ORDER - numofrejection, NUM_Scheules, numofrejection, NUM_DAY);
    fprintf(file, "***Accepted Orders***\n");
    for (int i = 0; i < NUM_DAY; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (schedules[i][j].quantity > 0)
            {
                fprintf(file, "%s %s %s %d %s\n",
                        schedules[i][j].date, schedules[i][j].productId, schedules[i][j].orderId,
                        schedules[i][j].quantity, schedules[i][j].plantId);
            }
        }
    }
    fprintf(file, "***Rejected Orders***\n");
    for (int i = 0; i < numofrejection; i++)
    {
        fprintf(file, "%s %s %s %d\n",
                rejected[i].date, rejected[i].productId, rejected[i].orderId,
                rejected[i].quantity);
    }
    fclose(file);
}

Order *sortList(Order *head)
{
    if (head == NULL || head->next == NULL)
    {
        return head;
    }
    Order dummy = {NULL, "NA", "NA", 0, "NA"};
    Order *curr = &dummy;
    while (head != NULL)
    {
        Order *next = head->next;
        curr = insert(curr, head);
        head = next;
    }
    return dummy.next;
}

void runSRT(Order *head, char startDate[11], char endDate[11], const char *filename)
{
    Order *sortedHead = sortList(head);
    runFCFS(sortedHead, startDate, endDate, filename, false);
}

Order *insert(Order *head, Order *node)
{
    if (head == NULL || node->Quantity < head->Quantity)
    {
        node->next = head;
        return node;
    }
    Order *curr = head;
    while (curr->next != NULL && curr->next->Quantity <= node->Quantity)
    {
        curr = curr->next;
    }
    node->next = curr->next;
    curr->next = node;
    return head;
}

int addOrder(const char *orderId, const char *orderDate, int orderNum, const char *productId)
{

    // check the period of the order, if it's beyond the current period, then the system will directly reject it.
    char *enddate = pointertoperiod->start_date;
    struct tm order = {0}, due = {0};
    strptime(orderDate, "%Y-%m-%d", &order);
    strptime(enddate, "%Y-%m-%d", &due);
    time_t order_date = mktime(&order);
    time_t due_date = mktime(&due);
    int days = difftime(order_date, due_date) / (86400) + 1;
    if (days < 0)
    {
        printf("The period of the order is beyond the current period, please try another one.\n");
        return 0;
    }

    // check the product type of the oredr,if it's not defined, then the system will directly reject it.
    bool check_product_type = false;
    const char *validTypes[] = {"Product_A", "Product_B", "Product_C", "Product_D",
                                "Product_E", "Product_F", "Product_G", "Product_H", "Product_I"};
    int numValidTypes = sizeof(validTypes) / sizeof(validTypes[0]);
    for (int i = 0; i < numValidTypes; i++)
    {
        if (strcmp(productId, validTypes[i]) == 0)
        {
            check_product_type = true;
            break;
        }
    }
    if (!check_product_type)
    {
        printf("The productID of the order is not defined, please try another one.\n");
        return 0;
    }

    // check the order_number, if it has been used by another alr existed order, then the system will directly reject it.
    bool check_order_number = false;
    Order *current = head;
    while ((!check_order_number) && (current != NULL))
    {
        if (strcmp(current->Order_Number, orderId) == 0)
        {
            check_order_number = true;
            break;
        }
        current = current->next;
    }
    if (check_order_number)
    {
        printf("The orderID of the order has been used, please try another one.\n");
        return 0;
    }

    Order *newOrder = (Order *)malloc(sizeof(Order));
    strncpy(newOrder->Order_Number, orderId, sizeof(newOrder->Order_Number) - 1);
    newOrder->Order_Number[sizeof(newOrder->Order_Number) - 1] = '\0';
    strncpy(newOrder->Due_Date, orderDate, 11);
    newOrder->Due_Date[10] = '\0';
    newOrder->Quantity = orderNum;
    strncpy(newOrder->Product_Name, productId, sizeof(newOrder->Product_Name) - 1);
    newOrder->Product_Name[sizeof(newOrder->Product_Name) - 1] = '\0';
    newOrder->next = NULL;
    if (head == NULL)
    {
        head = newOrder;
    }
    else
    {
        Order *current = head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newOrder;
    }
    printf("Successfully added new order: [Order Number: %s, Due Date: %s, Quantity: %d, Product Name: %s\n", newOrder->Order_Number,
           newOrder->Due_Date, newOrder->Quantity, newOrder->Product_Name);
    return 1;
}

void addBatch(const char *filename)
{
    printf("***Adding batch from %s***\n", filename);
    // read the dat file and call addOrder for each line
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Failed to open batch file");
        return;
    }
    // Read each line from the file
    char line[100];
    while (fgets(line, sizeof(line), file))
    {
        // printf("The line is %s\n", line);
        char *token;
        char *command = strtok(line, " ");
        if (strcmp(command, "addPERIOD") == 0)
        {
            char *startDate = strtok(NULL, " ");
            char *endDate = strtok(NULL, "\n");
            struct tm start = {0}, end = {0};
            strptime(startDate, "%Y-%m-%d", &start);
            strptime(endDate, "%Y-%m-%d", &end);
            time_t start_time = mktime(&start);
            time_t end_time = mktime(&end);
            int days = difftime(end_time, start_time) / (86400) + 1;
            if (days < 0)
            {
                printf("The period you input doesn't exist, please try another one.\n");
                continue;
            }

            Period *period = (Period *)malloc(sizeof(Period));
            strncpy(period->start_date, startDate, 11);
            period->start_date[10] = '\0';
            strncpy(period->end_date, endDate, 11);
            period->end_date[10] = '\0';
            printf("Successfully added period starting from %s to %s.\n", period->start_date, period->end_date);
            pointertoperiod = period;
        }

        else if (strcmp(command, "addORDER") == 0)
        {
            char *orderId = strtok(NULL, " ");
            char *orderDate = strtok(NULL, " ");
            int orderNum = atoi(strtok(NULL, " "));
            char *productId = strtok(NULL, "\n");
            addOrder(orderId, orderDate, orderNum, productId);
        }

        else if (strcmp(command, "runPLS") == 0)
        {

            char *algorithm = strtok(NULL, " ");
            char *PIPE_SYMBOL = strtok(NULL, " ");
            char *reportCommand = strtok(NULL, " >");
            char *skip = strtok(NULL, " ");
            char *filename = strtok(NULL, "\n");

            if (algorithm == NULL)
            {
                printf("No algorithm provided.\n");
            }
            else
            {
                int p[2];
                if (pipe(p) < 0)
                {
                    printf("Pipe creation error\n");
                    exit(1);
                }

                pid_t pid = fork();
                if (pid == 0)
                {
                    // scheduler
                    if (strcmp(algorithm, "FCFS") == 0)
                    {
                        runFCFS(head, pointertoperiod->start_date, pointertoperiod->end_date, filename, true);
                    }
                    else if (strcmp(algorithm, "SRT") == 0)
                    {
                        runSRT(head, pointertoperiod->start_date, pointertoperiod->end_date, filename);
                    }

                    else if (strcmp(algorithm, "Oracle") == 0)
                    {
                        Oracle(head, pointertoperiod->start_date, pointertoperiod->end_date, filename);
                    }
                    else
                    {
                        printf("Unsupported algorithm.\n");
                    }
                    if (PIPE_SYMBOL != NULL && strcmp(PIPE_SYMBOL, "|") == 0 && reportCommand != NULL && strcmp(reportCommand, "printREPORT") == 0 && filename != NULL)
                    {
                        FILE *file = freopen(filename, "w", stdout);
                        if (file)
                        {
                            char input_file[256];
                            sprintf(input_file, "%s_tmp.txt", filename); // create the temp name waiting for being passed to printReport
                            printReport(input_file, filename);
                            fclose(file);
                            remove(input_file); // remove the temp file
                        }
                        else
                        {
                            perror("Failed to open file");
                        }
                    }
                    exit(0);
                }
                waitpid(pid, NULL, 0);
            }
        }
    }
    fclose(file);
}

// comparison method between two accepted orders, used to sort
int compareACKOrderID(const void *a, const void *b)
{
    const Ack_Detail *detailA = (const Ack_Detail *)a;
    const Ack_Detail *detailB = (const Ack_Detail *)b;
    return strcmp(detailA->orderID, detailB->orderID);
}

// comparison method between two rejected orders, used to sort
int compareREJOrderID(const void *a, const void *b)
{
    const Rej_Detail *detailA = (const Rej_Detail *)a;
    const Rej_Detail *detailB = (const Rej_Detail *)b;
    return strcmp(detailA->orderID, detailB->orderID);
}

void printReport(const char *input, const char *filename)
{
    char buffer[1024];
    char algr[50];
    FILE *input_file = fopen(input, "r");
    if (input_file == NULL)
    {
        perror("Error opening input file");
        return;
    }

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }
    // print out the header
    fgets(buffer, 1024, input_file);
    if (sscanf(buffer, "***Production Schedule: %[^*]", algr) == 1)
    {
        fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        fprintf(file, "|  ***%s Schedule Analysis Report***                                            |\n", algr);
        fprintf(file, "|                                                                                   |\n");
        // check the algr used currently
        if (strcmp(algr, "FCFS") == 0)
        {
            fprintf(file, "|  Algorithm used: First Come First Served                                          |\n\n");
            fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        }
        else if (strcmp(algr, "SRT") == 0)
        {
            fprintf(file, "|  Algorithm used: Shortest Remaining Time                                          |\n\n");
            fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        }
        else if (strcmp(algr, "Oracle") == 0)
        {
            fprintf(file, "|  Algorithm used: Oracle                                                           |\n");
            fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        }
        else
        {
            fprintf(file, "|  Ops, we don't have the algorithm                                                 |");
            fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        }
    }
    int ack_orders, rej_orders;     // counter for orders
    int ack_schs, num_day;          // counter for schedules
    Schedule *schs = NULL;          // store the received schedules
    Schedule *rejs = NULL;          // store the received rejected orders
    Ack_Detail *details = NULL;     // initialize the print entry
    Rej_Detail *rej_details = NULL; // initialize the print entry
    int detail_count = 0;           // counter for print entries
    fgets(buffer, 1024, input_file);
    if (sscanf(buffer, "***Accepted Order: %d, Accepted Sch: %d, Rejected Order: %d, Total Days: %d***", &ack_orders, &ack_schs, &rej_orders, &num_day) == 4)
    {
        // pre-allocate the memory
        schs = malloc(ack_schs * sizeof(Schedule));
        rejs = malloc(rej_orders * sizeof(Schedule));
        details = malloc(ack_schs * sizeof(Ack_Detail));
        rej_details = malloc(rej_orders * sizeof(Rej_Detail));
    }

    // process of accepted orders
    fgets(buffer, 1024, input_file);
    if (strstr(buffer, "***Accepted Orders***") != NULL)
    {
        fprintf(file, "             >  There are %d Orders ACCEPTED.  Details are as follows:  <\n", ack_orders);
        fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        // collect the data, storing to the sch array, ready for merging
        for (int i = 0; i < ack_schs; i++)
        {
            if (fgets(buffer, 1024, input_file) && sscanf(buffer, "%s %s %s %d %s",
                                                          schs[i].date,
                                                          schs[i].productId,
                                                          schs[i].orderId,
                                                          &schs[i].quantity,
                                                          schs[i].plantId) == 5)
            {
                // no further operation here
            }
            else
            {
                fprintf(file, "|  error in ack order process                                                       |");
                break;
            }
        }

        // merge part
        for (int i = 0; i < ack_schs; i++)
        {
            int found_flag = 0;
            for (int j = 0; j < detail_count; j++)
            {

                // items that could be merged： identical orderID and plantID
                if (strcmp(schs[i].orderId, details[j].orderID) == 0 && strcmp(schs[i].plantId, details[j].plantID) == 0)
                {
                    found_flag = 1;
                    if (strcmp(schs[i].date, details[j].start) < 0) // update the start date
                    {
                        strcpy(details[j].start, schs[i].date);
                    }
                    if (strcmp(schs[i].date, details[j].start) > 0) // update the end date
                    {
                        strcpy(details[j].end, schs[i].date);
                    }
                    details[j].quantity += schs[i].quantity; // cumulate the quantity
                    details[j].days += 1;                    // increment to the number of days
                    break;
                }
            }
            // items cannot be merged or appear for the first time
            if (!found_flag && detail_count < ack_schs)
            {
                strcpy(details[detail_count].orderID, schs[i].orderId);
                strcpy(details[detail_count].plantID, schs[i].plantId);
                strcpy(details[detail_count].start, schs[i].date);
                strcpy(details[detail_count].end, schs[i].date);
                details[detail_count].quantity = schs[i].quantity;
                details[detail_count].days = 1; // initialize the number of days
                detail_count++;
            }
        }
        // sort by orderID
        qsort(details, detail_count, sizeof(Ack_Detail), compareACKOrderID);
        // print out the header
        fprintf(file, "|  ORDER NUMBER   START          END               DAYS    QUANTITY     PLANT       |\n");
        fprintf(file, "|  ==============================================================================   |\n");

        // print out each entry
        for (int i = 0; i < detail_count; i++)
        {
            fprintf(file, "|  %-14s %-14s %-14s %7d %11d     %-10s  |\n",
                    details[i].orderID,
                    details[i].start,
                    details[i].end,
                    details[i].days,
                    details[i].quantity,
                    details[i].plantID);
        }

        // print out the footer
        fprintf(file, "|                                   - End -                                         |\n");
        fprintf(file, "|                                                                                   |\n");
        fprintf(file, "|-----------------------------------------------------------------------------------|\n\n");
    }

    // process of rejected orders
    fgets(buffer, 1024, input_file);
    if (strstr(buffer, "***Rejected Orders***") != NULL)
    {
        fprintf(file, "             >  There are %d Orders REJECTED.  Details are as follows:  <\n\n", rej_orders);
        fprintf(file, "|-----------------------------------------------------------------------------------|\n");
        // collect the data, storing to the detail array, no need for merging
        for (int i = 0; i < rej_orders; i++)
        {
            if (fgets(buffer, 1024, input_file) && sscanf(buffer, "%s %s %s %d",
                                                          rej_details[i].date,
                                                          rej_details[i].productID,
                                                          rej_details[i].orderID,
                                                          &rej_details[i].quantity) == 4)
            {
                // no further operation here
            }
            else
            {
                fprintf(file, "|  error in rejected order process                                                  |");
                break;
            }
        }

        // sort by orderID
        qsort(rej_details, rej_orders, sizeof(Rej_Detail), compareREJOrderID);

        // print out the header
        fprintf(file, "|  ORDER NUMBER         PRODUCT NAME               Due Date          QUANTITY       |\n");
        fprintf(file, "|  ==============================================================================   |\n");

        // print out each entry
        for (int i = 0; i < rej_orders; i++)
        {
            fprintf(file, "|  %-20s %-25s %11s %15d       |\n",
                    rej_details[i].orderID,
                    rej_details[i].productID,
                    rej_details[i].date,
                    rej_details[i].quantity);
        }
        // print out the footer
        fprintf(file, "|                                   - End -                                         |\n");
        fprintf(file, "|                                                                                   |\n");
        fprintf(file, "|-----------------------------------------------------------------------------------|\n\n");
    }

    // the process of performance
    fprintf(file, "                              >***PERFORMANCE***<                                                                \n\n");
    // 0 -> X, 1 -> Y, 2 -> Z
    int counts_days[3] = {0};
    int counts_products[3] = {0};
    int counts_could_produce[3] = {num_day * 300, num_day * 400, num_day * 500};
    // collect datas from the schs array
    for (int i = 0; i < ack_schs; i++)
    {
        if (strcmp(schs[i].plantId, "Plant_X") == 0)
        {
            counts_days[0] += 1;
            counts_products[0] += schs[i].quantity;
        }
        else if (strcmp(schs[i].plantId, "Plant_Y") == 0)
        {
            counts_days[1] += 1;
            counts_products[1] += schs[i].quantity;
        }
        else
        {
            counts_days[2] += 1;
            counts_products[2] += schs[i].quantity;
        }
    }

    // calculate and print
    fprintf(file, "     >  Plant_X:  <\n");
    fprintf(file, "     >  Number of days in use:       %d days  <\n", counts_days[0]);
    fprintf(file, "     >  Number of products produced: %d (in total)  <\n", counts_products[0]);
    float utilizationX = (float)counts_products[0] / counts_could_produce[0] * 100;
    fprintf(file, "     >  Utilization of the plant:    %.1f %%  <\n\n", utilizationX);

    fprintf(file, "     >  Plant_Y:  <\n");
    fprintf(file, "     >  Number of days in use:       %d days  <\n", counts_days[1]);
    fprintf(file, "     >  Number of products produced: %d (in total)  <\n", counts_products[1]);
    float utilizationY = (float)counts_products[1] / counts_could_produce[1] * 100;
    fprintf(file, "     >  Utilization of the plant:    %.1f %%  <\n\n", utilizationY);

    fprintf(file, "     >  Plant_Z:  <\n");
    fprintf(file, "     >  Number of days in use:       %d days  <\n", counts_days[2]);
    fprintf(file, "     >  Number of products produced: %d (in total)  <\n", counts_products[2]);
    float utilizationZ = (float)counts_products[2] / counts_could_produce[2] * 100;
    fprintf(file, "     >  Utilization of the plant:    %.1f %%  <\n\n", utilizationZ);

    float overall_utilization = (utilizationX + utilizationY + utilizationZ) / 3;
    fprintf(file, "     >  Overall of utilization:          %.1f %%  <\n\n\n", overall_utilization);

    // free and close
    free(schs);
    free(rejs);
    free(details);
    free(rej_details);
    fclose(file);
    fclose(input_file);
}