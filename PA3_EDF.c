#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char task_name[2];
    int deadline;
    int WCET_1188;
    int WCET_918;
    int WCET_648;
    int WCET_384;
    int next_deadline;
    int ready;
    int remaining_execution_time;

} TaskObject; // Object for holding task specific values

typedef struct {
    int system_ex_time;
    int CPU_1188;
    int CPU_918;
    int CPU_648;
    int CPU_384;
    int CPU_idle;
} SystemSpecs; // System Specifications Object

typedef struct ReadyList{
    TaskObject *task;
    struct ReadyList *ptr;
} ReadyList;

#define EE_SETTING 1
#define DEFAULT 0

static ReadyList* insert(ReadyList* head, TaskObject *task){ //insert task into ready list sorted by deadline (shortest deadline more priority)
    ReadyList *tmp, *prev, *next;
    tmp = (ReadyList*)malloc(sizeof(ReadyList));
    tmp->task = task;
    tmp->ptr = NULL;
    if(head == NULL) {
        head = tmp;
    } else {
        next = head;
        prev = NULL;

        while(next!=NULL && next->task->deadline<=task->deadline){
            prev = next;
            next = next->ptr;
        }
        if(next == NULL) {
            prev->ptr = tmp;
        } else {
            if(prev){
                tmp->ptr = prev->ptr;
                prev->ptr = tmp;
            } else {
                tmp->ptr = head;
                head = tmp;
            }
        }
    }
    return head;
}

static ReadyList* runRM(ReadyList *head, SystemSpecs system, int *time, float *total_enegry){
    TaskObject *task = head->task;
    float energy = ((float)system.CPU_1188*(float)task->WCET_1188)*0.001;
    printf("%d %s %d %d %fJ\n", *time, task->task_name, 1188, task->WCET_1188, energy);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;
    *total_enegry += energy; 
    *time += task->WCET_1188;
    return newHead;
}

static ReadyList* runEDF(ReadyList *head, SystemSpecs system, int *time, float *total_energy){
    if (head == NULL) {
        return NULL;
    }

    TaskObject *task = head->task;
    int execution_time = task->WCET_1188; // Assuming highest CPU speed for simplicity
    float energy = ((float)system.CPU_1188 * (float)execution_time) * 0.001;

    printf("%d %s %d %d %fJ\n", *time, task->task_name, 1188, execution_time, energy);

    task->next_deadline += task->deadline; // Update next deadline
    task->ready = 0; // Mark task as not ready

    *total_energy += energy;
    *time += execution_time; // Update the system time

    ReadyList *newHead = head->ptr; // Move to the next task in the ready list
    free(head); // Free the memory allocated to the current head

    return newHead;
}
static void EDF_schedule(TaskObject tasks[], SystemSpecs system, int num_tasks) {
    ReadyList *readylist = NULL;
    int time = 0;
    float total_energy_consumption = 0;

    while(time < system.system_ex_time){
        for (int j = 0; j < num_tasks; j++) {
            if (time >= tasks[j].next_deadline && tasks[j].ready == 0){
                tasks[j].ready = 1;
                readylist = insert(readylist, &tasks[j]); // Insert arrived tasks to ready list
            }
        }

        if(readylist){
            readylist = runEDF(readylist, system, &time, &total_energy_consumption); // Run the tasks in ready list using EDF
        } else {
            // Handle idle time and energy consumption when no task is ready
            // Similar logic as in RM_schedule
        }
    }

    // Print total energy consumption and other stats
}


static ReadyList* runRM_EE(ReadyList *head, SystemSpecs system, int *time, float *total_energy){
    TaskObject *task = head->task;
    
    return NULL;
}

static void RM_schedule(TaskObject tasks[], SystemSpecs system, int num_tasks, int Setting) {
    ReadyList *readylist = NULL;
    int time = 0;
    int idleTime = 0;
    int total_idle_time = 0;
    float total_energy_consumption = 0;

    //excecute the periodic tasks to maximum time
    while(time < system.system_ex_time){
        for (int j = 0; j < num_tasks; j++) {
            if (time >= tasks[j].next_deadline && tasks[j].ready == 0){
                tasks[j].ready = 1;
                readylist = insert(readylist, &tasks[j]); //insert arrived tasks to ready list

            }
        }
        if(readylist){
            if (Setting){

            } else {
            readylist = runRM(readylist, system, &time, &total_energy_consumption); //run the tasks in ready list
            }
        } else {
            int closestDeadline = INT32_MAX;
            int index = 0;
            for (int k = 0; k < num_tasks; k++) {
                if (tasks[k].next_deadline - time < closestDeadline) { //calculate idle time by seeing when the next task will arrive
                    closestDeadline = tasks[k].next_deadline - time;
                    index = k;
                }
            }
            int idle_time = tasks[index].next_deadline - time;
            total_idle_time += idle_time;
            float energy = (float)idle_time*(float)system.CPU_idle*0.001;
            printf("%d IDLE IDLE %d %fJ\n", time, idle_time, energy);
            total_energy_consumption += energy;
            time = tasks[index].next_deadline;
        }
    }

    printf("%fJ %f%% %ds\n", total_energy_consumption, (float)total_idle_time/(float)system.system_ex_time, system.system_ex_time);


}

int main(int argc, char **argv){
    FILE *file;
    file = fopen("input1.txt", "r");

    int num_tasks;
    SystemSpecs system;

    if (file == NULL){
        printf("Couldn't open file");
        exit(1);
    }

    fscanf(file, "%d %d %d %d %d %d %d", &num_tasks, &system.system_ex_time, &system.CPU_1188, &system.CPU_918, &system.CPU_648, &system.CPU_384, &system.CPU_idle); //grabs first row of file

    TaskObject taskArray[num_tasks]; // initialize array of task objects

    for(int i = 0; i < num_tasks; i++){
        fscanf(file, "%s %d %d %d %d %d", taskArray[i].task_name, &taskArray[i].deadline, &taskArray[i].WCET_1188, &taskArray[i].WCET_918, &taskArray[i].WCET_648, &taskArray[i].WCET_384);
        taskArray[i].next_deadline = 0;
        taskArray[i].ready = 0;
        taskArray[i].remaining_execution_time = taskArray[i].WCET_1188; // Assuming using WCET_1188 for simplicity
        // printf("%s %d %d %d %d %d\n", taskArray[i].task_name, taskArray[i].deadline, taskArray[i].WCET_1188, taskArray[i].WCET_918, taskArray[i].WCET_648, taskArray[i].WCET_384);
    }

    //RM_schedule(taskArray, system, num_tasks, DEFAULT);
    EDF_schedule(taskArray, system, num_tasks); // Call EDF scheduler

    // main functionality
    // if (strcmp(argv[2], "RM") == 0) {
    //     if (strcmp(argv[3], "EE") == 0) { // RM EE

    //     } else { // RM
    //         RM_schedule(tasks, system);
    //         exit(0);
    //     }
    // } else if (strcmp(argv[2], "EDF") == 0) {
    //     if (strcmp(argv[3], "EE") == 0) { //EDF EE

    //     } else { // EDF

    //     }
    // } else {
    //     printf("Invalid Arguements");
    //     exit(1);
    // }

    return 0;
}