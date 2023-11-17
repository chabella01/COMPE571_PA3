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

static ReadyList* insert(ReadyList* head, TaskObject *task){ //insert task into ready list sorted by deadline
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

static ReadyList* runRM(ReadyList *head, SystemSpecs system, int time){
    TaskObject *task = head->task;
    printf("%d %s %d %d %fJ\n", time, task->task_name, 1188, task->WCET_1188, ((float)system.CPU_1188*(float)task->WCET_1188)*0.001);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;

    return newHead;
}

static void RM_schedule(TaskObject tasks[], SystemSpecs system) {
    ReadyList *readylist = NULL;
    int wait_time[5] = {0, 0, 0, 0, 0};
    int time = 0;
    int idleTime = 0;
    while(time < system.system_ex_time){
        for (int j = 0; j < 5; j++) {
            if (time >= tasks[j].next_deadline && tasks[j].ready == 0){
                tasks[j].ready = 1;
                readylist = insert(readylist, &tasks[j]);

            }
        }
        if(readylist){
            int newTime = time + readylist->task->WCET_1188;
            readylist = runRM(readylist, system, time);
            time = newTime;
        } else {
            int closestDeadline = INT32_MAX;
            int index = 0;
            for (int k = 0; k < 5; k++) {
                if (tasks[k].next_deadline - time < closestDeadline) {
                    closestDeadline = tasks[k].next_deadline - time;
                    index = k;
                }
            }
            int idle_time = tasks[index].next_deadline - time;
            printf("%d IDLE IDLE %d %fJ\n", time, idle_time, (float)idle_time*(float)system.CPU_idle*0.001);
            time = tasks[index].next_deadline;
        }
    }


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
        // printf("%s %d %d %d %d %d\n", taskArray[i].task_name, taskArray[i].deadline, taskArray[i].WCET_1188, taskArray[i].WCET_918, taskArray[i].WCET_648, taskArray[i].WCET_384);
    }

    RM_schedule(taskArray, system);

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