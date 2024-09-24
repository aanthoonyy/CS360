#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Multiplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read

    uint8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    uint32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    uint32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    uint32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)

    uint32_t IOBurst;                   // The amount of time until the process finishes being blocked
    uint32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption

    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode

    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
} _process;


uint32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
uint32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total cycles in the blocked state

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers";
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file

// Additional variables as needed
/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * process_list The original processes inputted, in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function


// ---------------- AHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
typedef struct Node {
    _process *process;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

Queue *createQueue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue *q, _process *process) {
    Node *temp = (Node*)malloc(sizeof(Node));
    temp->process = process;
    temp->next = NULL;

    if (q == NULL) {
    printf("Error: Queue is NULL\n");
    return;
    }
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }

    q->rear->next = temp;
    q->rear = temp;
}

_process *dequeue(Queue *q) {
    if (q->front == NULL)
        return NULL;

    Node *temp = q->front;
    _process *process = temp->process;

    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;

    free(temp);

    return process;
}

void sortQueue(Queue* queue) {
Node* node = queue->front;
int size = 0;
while (node != NULL) {
    node = node->next;
    size++;
}
    _process* arr[size];

    // Transfer elements from queue to array
    for (int i = 0; i < size; i++) {
        arr[i] = dequeue(queue);
    }

    // Sort array using bubble sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j]->B > arr[j + 1]->B) {
                _process* temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }

    // Transfer elements from array back to queue
    for (int i = 0; i < size; i++) {
        enqueue(queue, arr[i]);
    }
}

/**
 * The magic starts from here
 */
int main(int argc, char *argv[]) {
    // Placeholder for reading the total number of processes, for example, from stdin or a file
    uint32_t total_num_of_process;
    total_num_of_process = 3;
    Queue* readyQueueHead = createQueue();
    Queue* blockedQueueHead = createQueue();
    _process* runningProcess = NULL;
    // Dynamically allocate memory for process_list based on the input
    _process* process_list = (_process*)malloc(total_num_of_process * sizeof(_process));
    if (process_list == NULL) {
        printf("Memory allocation failed.\n");
        return 1; // Exit if memory allocation fails
    }

    // Placeholder: Read process data into process_list here...
    // For simplicity, let's assume this data is already populated.
    // Normally, you would read this data from a file or stdin.

    FILE* random = fopen("random-numbers", "r");
    FILE* f = fopen(argv[1],"r");
    if (f == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    char line[256];
    char* buffer;
    int arrival_time;
    int cpu_burst_time_bound;
    int total_cpu_time_required;
    int multiplier;
    fgets(line, sizeof(line), f);
    char line2[256];
    strcpy(line2, line);
    buffer = strtok(line2, " ");
    //printf("The total number of processes is: %s\n", buffer);
    int tnop = atoi(buffer);
    total_num_of_process = tnop;
    //printf("got to here\n");
// parsing
for (uint32_t i = 0; i < total_num_of_process; i++) {
    if (i == 0) {
        buffer = strtok(line, "(");
    }
    buffer = strtok(NULL, " ");
    process_list[i].A = atoi(buffer);
    buffer = strtok(NULL, " ");
    process_list[i].B = atoi(buffer);
    buffer = strtok(NULL, " ");
    process_list[i].C = atoi(buffer);
    buffer = strtok(NULL, ")");
    process_list[i].M = atoi(buffer);
    process_list[i].processID = i;
    process_list[i].status = 0;
    process_list[i].finishingTime = -1; 
    process_list[i].currentCPUTimeRun = 0;
    process_list[i].currentIOBlockedTime = 0;
    process_list[i].currentWaitingTime = 0;
    process_list[i].IOBurst = 0;
    process_list[i].CPUBurst = randomOS(process_list[i].B, i, random);
    process_list[i].quantum = -1;
    process_list[i].isFirstTimeRunning = true;
    process_list[i].nextInBlockedList = NULL;
    process_list[i].nextInReadyQueue = NULL;
    process_list[i].nextInReadySuspendedQueue = NULL;


    if (i < total_num_of_process - 1) {
        strtok(NULL, "(");
    }
}
    // Main simulation loop for FCFS scheduling
    uint32_t current_cycle = 0;
    uint32_t total_finished_processes = 0;
    uint32_t process_index = 0;
    uint32_t currentBurst = 0;
    int scheduling_algorithm = 0;
    for ( scheduling_algorithm = 0; scheduling_algorithm < 3; scheduling_algorithm++){
    switch (scheduling_algorithm){
        case 0:
    // ---------------------------------------------------------
    // FCFS scheduling
    // ---------------------------------------------------------
    TOTAL_CREATED_PROCESSES = total_num_of_process;
    uint32_t completed_process_index = 0;
    
        while (total_finished_processes < total_num_of_process) {
        // Step 1: Check for new arrivals and enqueue them
        for (int i = 0; i < total_num_of_process; i++) {
            if (process_list[i].A == current_cycle && process_list[i].status == 0) {
                process_list[i].status = 1; // Mark as ready
                enqueue(readyQueueHead, &process_list[i]);
                //process_list[i].currentWaitingTime = current_cycle;
                //addProcessToReadyQueue(&readyQueueHead, &process_list[i]);

            }
        }

    // Step 2: If no process is running, dequeue the next process and run it
    if (runningProcess == NULL) {
        //runningProcess = removeProcessFromReadyQueue(&readyQueueHead);
        runningProcess = dequeue(readyQueueHead);
        
        if (runningProcess != NULL) {
            currentBurst = runningProcess->CPUBurst;
            runningProcess->status = 2; // Mark as running
        }
    }

    // Step 3: Update running process
    if (runningProcess != NULL) {
        runningProcess->currentCPUTimeRun++;
        // Check if the running process completes
        //runningProcess->currentCPUTimeRun++;
        if (runningProcess->currentCPUTimeRun == runningProcess->C) {
            runningProcess->status = 4; // Mark as finished
            
            runningProcess->currentWaitingTime--;
            runningProcess->finishingTime = runningProcess->C + runningProcess->currentIOBlockedTime + runningProcess->currentWaitingTime;
            //dequeue(readyQueueHead);
            total_finished_processes++;
            runningProcess = NULL; // No process is running now
        } 
        else if (runningProcess->currentCPUTimeRun % runningProcess->B == 0) {
            // Process needs to be blocked for I/O
            runningProcess->status = 3; // Mark as blocked
            //addProcessToBlockedList(&blockedQueueHead, &process_list[runningProcess->processID]);
            runningProcess->currentWaitingTime++;
            TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED++;
            enqueue(blockedQueueHead, &process_list[runningProcess->processID]);
            // total_finished_processes++;
            //runningProcess->IOBurst = randomOS(runningProcess->B, runningProcess->processID, f); // Assuming this function exists
            // runningProcess = NULL;
        } 
    }

    // Step 4: Update blocked processes
    for (int i = 0; i < total_num_of_process; i++) {
        if (process_list[i].status == 3) { // If the process is blocked
            process_list[i].IOBurst--;
            
            process_list[i].currentIOBlockedTime++;
            if (process_list[i].IOBurst == 0) {
                process_list[i].status = 1; // Move back to ready
                
                //addProcessToReadyQueue(&readyQueueHead, removeProcessFromBlockedList(&runningProcess));
                enqueue(readyQueueHead, dequeue(blockedQueueHead));
            }
        }
        if (process_list[i].status == 1){
            process_list[i].currentWaitingTime++;
        }
        if (process_list[i].status == 2){
            runningProcess->CPUBurst = randomOS(process_list[i].B, process_list[i].processID, random);
            currentBurst--;
        }
    }

    current_cycle++;
}
    CURRENT_CYCLE = current_cycle;
    //printf("here");
    printf("######################### START OF FIRST COME FIRST SERVE #########################\n");
    printStart(process_list);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF FIRST COME FIRST SERVE #########################\n");
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    //free(process_list);
    break;

    case 1:
//     // ---------------------------------------------------------
//     // SHORTEST JOB FIRST scheduling
//     // ---------------------------------------------------------
    TOTAL_CREATED_PROCESSES = total_num_of_process;
    completed_process_index = 0;
    
        while (total_finished_processes < total_num_of_process) {
        // Step 1: Check for new arrivals and enqueue them
        for (int i = 0; i < total_num_of_process; i++) {
            if (process_list[i].A == current_cycle && process_list[i].status == 0) {
                process_list[i].status = 1; // Mark as ready
                enqueue(readyQueueHead, &process_list[i]);
                
                //process_list[i].currentWaitingTime = current_cycle;
                //addProcessToReadyQueue(&readyQueueHead, &process_list[i]);

            }
        }
    sortQueue(readyQueueHead);
    // Step 2: If no process is running, dequeue the next process and run it
    if (runningProcess == NULL) {
        //runningProcess = removeProcessFromReadyQueue(&readyQueueHead);
        runningProcess = dequeue(readyQueueHead);
        
        if (runningProcess != NULL) {
            currentBurst = runningProcess->CPUBurst;
            runningProcess->status = 2; // Mark as running
        }
    }

    // Step 3: Update running process
    if (runningProcess != NULL) {
        runningProcess->currentCPUTimeRun++;
        // Check if the running process completes
        //runningProcess->currentCPUTimeRun++;
        if (runningProcess->currentCPUTimeRun == runningProcess->C) {
            runningProcess->status = 4; // Mark as finished
            runningProcess->finishingTime = current_cycle + 1;
            
            total_finished_processes++;
            runningProcess = NULL; // No process is running now
        } 
        else if (runningProcess->currentCPUTimeRun % runningProcess->B == 0) {
            // Process needs to be blocked for I/O
            runningProcess->status = 3; // Mark as blocked
            //addProcessToBlockedList(&blockedQueueHead, &process_list[runningProcess->processID]);
            runningProcess->currentWaitingTime++;
            TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED++;
            enqueue(blockedQueueHead, &process_list[runningProcess->processID]);
            // total_finished_processes++;
            //runningProcess->IOBurst = randomOS(runningProcess->B, runningProcess->processID, f); // Assuming this function exists
            // runningProcess = NULL;
        } 
    }

    // Step 4: Update blocked processes
    for (int i = 0; i < total_num_of_process; i++) {
        if (process_list[i].status == 3) { // If the process is blocked
            process_list[i].IOBurst--;
            
            process_list[i].currentIOBlockedTime++;
            if (process_list[i].IOBurst == 0) {
                process_list[i].status = 1; // Move back to ready
                
                //addProcessToReadyQueue(&readyQueueHead, removeProcessFromBlockedList(&runningProcess));
                enqueue(readyQueueHead, dequeue(blockedQueueHead));
            }
        }
        if (process_list[i].status == 1){
            process_list[i].currentWaitingTime++;
        }
        if (process_list[i].status == 2){
            
            currentBurst--;
        }
    }

    current_cycle++;
}
    CURRENT_CYCLE = current_cycle;
    //printf("here");
    printf("######################### START OF SHORTEST JOB FIRST #########################\n");
    printStart(process_list);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF SHORST JOB FIRST #########################\n");
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    //free(process_list);
    break;
    case 2:
    // ---------------------------------------------------------
    // ROUND ROBIN scheduling
    // ---------------------------------------------------------
    TOTAL_CREATED_PROCESSES = total_num_of_process;
    completed_process_index = 0;
    
        while (total_finished_processes < total_num_of_process) {
        // Step 1: Check for new arrivals and enqueue them
        for (int i = 0; i < total_num_of_process; i++) {
            if (process_list[i].A == current_cycle && process_list[i].status == 0) {
                process_list[i].status = 1; // Mark as ready
                enqueue(readyQueueHead, &process_list[i]);
                
                //process_list[i].currentWaitingTime = current_cycle;
                //addProcessToReadyQueue(&readyQueueHead, &process_list[i]);

            }
        }
    
    // Step 2: If no process is running, dequeue the next process and run it
    if (runningProcess == NULL) {
        //runningProcess = removeProcessFromReadyQueue(&readyQueueHead);
        runningProcess = dequeue(readyQueueHead);
        
        if (runningProcess != NULL) {
            currentBurst = runningProcess->CPUBurst;
            runningProcess->status = 2; // Mark as running
            runningProcess->quantum = 2;
        }
    }

    // Step 3: Update running process
    if (runningProcess != NULL) {
        runningProcess->currentCPUTimeRun++;
        runningProcess->quantum--;
        // Check if the running process completes
        //runningProcess->currentCPUTimeRun++;
        if (runningProcess->currentCPUTimeRun == runningProcess->C) {
            runningProcess->status = 4; // Mark as finished
            runningProcess->finishingTime = current_cycle + 1;
            current_cycle++;
            total_finished_processes++;
            runningProcess = NULL; // No process is running now
        } 
        else if (runningProcess->quantum == 0) {
            runningProcess->status = 1; // Mark as ready
            enqueue(readyQueueHead, runningProcess); // Add back to ready queue
            runningProcess = NULL; // No process is running now
        }
    }

    // Step 4: Update blocked processes
    for (int i = 0; i < total_num_of_process; i++) {
        if (process_list[i].status == 3) { // If the process is blocked
            process_list[i].IOBurst--;
            
            process_list[i].currentIOBlockedTime++;
            if (process_list[i].IOBurst == 0) {
                process_list[i].status = 1; // Move back to ready
                
                //addProcessToReadyQueue(&readyQueueHead, removeProcessFromBlockedList(&runningProcess));
                enqueue(readyQueueHead, dequeue(blockedQueueHead));
            }
        }
        if (process_list[i].status == 1){
            process_list[i].currentWaitingTime++;
        }
        if (process_list[i].status == 2){
            
            currentBurst--;
        }
    }

    current_cycle++;
}
    CURRENT_CYCLE = current_cycle;
    printf("######################### START OF FIRST COME ROUND ROBIN #########################\n");
    printStart(process_list);
    printFinal(process_list);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF FIRST COME ROUND ROBIN #########################\n");
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;
    //free(process_list);
    break;
    }
    }
    free(process_list);
    fclose(random);
    return 0;
}