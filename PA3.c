#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    char task_name[2];
    int deadline;
    int freq;
    int WCET_1188;
    int WCET_918;
    int WCET_648;
    int WCET_384;
    int next_deadline;
    int ready;
    int excecution_time;
    int power;

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

typedef struct {
    int times[4];
    int closest;
    int freq;
    int power;
} ExTimes;

#define EE_SETTING 1
#define DEFAULT 0

static double calculateExpression(int v, int w, int x, int y, int z) {
    return (double)v / 520 + (double)w / 220 + (double)x / 500 + (double)y / 200 + (double)z / 300;
}
static double calculatePowerConsumption(int ex, int cpufreq){
    return (double)ex * (double) cpufreq;
}
static ExTimes* findEE(TaskObject taskArray[], int num_tasks, SystemSpecs system){
    ExTimes* excecution_times = (ExTimes*)malloc(sizeof(ExTimes) * 5);
    int freqs[4] = {1188, 918, 648, 384};
    int powers[4] = {system.CPU_1188, system.CPU_918, system.CPU_648, system.CPU_384};
    for (int i = 0; i < num_tasks; i++) {
        excecution_times[i].times[0] = taskArray[i].WCET_1188;
        excecution_times[i].times[1] = taskArray[i].WCET_918;
        excecution_times[i].times[2] = taskArray[i].WCET_648;
        excecution_times[i].times[3] = taskArray[i].WCET_384;
    }

    double lowest_power = __DBL_MAX__;
    // static int closest[5];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                for (int l = 0; l < 4; l++) {
                    for (int m = 0; m < 4; m++) {
                        double current_value = calculateExpression(excecution_times[0].times[i], excecution_times[1].times[j], excecution_times[2].times[k], excecution_times[3].times[l], excecution_times[4].times[m]);
                        double difference = 1 - current_value;
                        double total_power = calculatePowerConsumption(excecution_times[0].times[i], powers[i]) + calculatePowerConsumption(excecution_times[1].times[j], powers[j]) + calculatePowerConsumption(excecution_times[2].times[k], powers[k]) + calculatePowerConsumption(excecution_times[3].times[l], powers[l]) + calculatePowerConsumption(excecution_times[4].times[m], powers[i]);
                        // Check if the current combination is closer than the previous closest
                        if (difference > 0  && total_power < lowest_power) {
                            lowest_power = total_power;

                            excecution_times[0].closest = excecution_times[0].times[i];
                            excecution_times[0].freq = freqs[i];
                            excecution_times[0].power = powers[i];

                            excecution_times[1].closest = excecution_times[1].times[j];
                            excecution_times[1].freq = freqs[j];
                            excecution_times[1].power = powers[j];

                            excecution_times[2].closest = excecution_times[2].times[k];
                            excecution_times[2].freq = freqs[k];
                            excecution_times[2].power = powers[k];

                            excecution_times[3].closest = excecution_times[3].times[l];
                            excecution_times[3].freq = freqs[l];
                            excecution_times[3].power = powers[l];

                            excecution_times[4].closest = excecution_times[4].times[m];
                            excecution_times[4].freq = freqs[m];
                            excecution_times[4].power = powers[m];

                        }
                    }
                }
            }
        }
    }
    return excecution_times;
}
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

static ReadyList* runRM(ReadyList *head, SystemSpecs system, int *time, float *total_energy){
    TaskObject *task = head->task;
    float energy = ((float)system.CPU_1188*(float)task->excecution_time)*0.001;
    printf("%d %s %d %d %fJ\n", *time, task->task_name, 1188, task->excecution_time, energy);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;
    *total_energy += energy; 
    *time += task->excecution_time;
    return newHead;
}

static ReadyList* runRM_EE(ReadyList *head, int CPU_freq, int CPU_power, int *time, float *total_energy){
    TaskObject *task = head->task;
    float energy = ((float)CPU_power*(float)task->excecution_time)*0.001;
    printf("%d %s %d %d %fJ\n", *time, task->task_name, CPU_freq, task->excecution_time, energy);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;
    *total_energy += energy; 
    *time += task->excecution_time;
    return newHead;
}

static void RM_schedule(TaskObject tasks[], SystemSpecs system, int num_tasks, int Setting) {
    ReadyList *readylist = NULL;
    int time = 0;
    int idleTime = 0;
    int total_idle_time = 0;
    float total_energy_consumption = 0;

    double exponent = 1 / (double)num_tasks;
    float max_util = (float)num_tasks * (pow(2, exponent) - 1);

    if (Setting) {
        TaskObject taskArr[] = {tasks[0], tasks[1], tasks[2], tasks[3], tasks[4]};
        ExTimes *c_times = findEE(taskArr, num_tasks, system);
        for (int i = 0; i < num_tasks; i++) {
            tasks[i].excecution_time = c_times[i].closest;
            tasks[i].freq = c_times[i].freq;
            tasks[i].power = c_times[i].power;

        }
    }
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
            readylist = runRM_EE(readylist, readylist->task->freq, readylist->task->power, &time, &total_energy_consumption);
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
        taskArray[i].excecution_time = taskArray[i].WCET_1188;
        taskArray[i].freq = 1188;
        taskArray[i].power = 0;
        // printf("%s %d %d %d %d %d\n", taskArray[i].task_name, taskArray[i].deadline, taskArray[i].WCET_1188, taskArray[i].WCET_918, taskArray[i].WCET_648, taskArray[i].WCET_384);
    }

    RM_schedule(taskArray, system, num_tasks, EE_SETTING);

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