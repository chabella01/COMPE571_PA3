#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    char task_name[2];
    int deadline;
    int WCET_1188;
    int WCET_918;
    int WCET_648;
    int WCET_384;
    int next_deadline;
    int ready;
} TaskObject;

typedef struct {
    int system_ex_time;
    int CPU_1188;
    int CPU_918;
    int CPU_648;
    int CPU_384;
    int CPU_idle;
} SystemSpecs;

typedef struct ReadyList {
    TaskObject *task;
    struct ReadyList *ptr;
} ReadyList;

#define EE_SETTING 1
#define DEFAULT 0

static ReadyList* insert(ReadyList* head, TaskObject *task) {
    ReadyList *tmp, *prev, *next;
    tmp = (ReadyList*)malloc(sizeof(ReadyList));
    tmp->task = task;
    tmp->ptr = NULL;
    if (head == NULL) {
        head = tmp;
    } else {
        next = head;
        prev = NULL;
        while (next != NULL && next->task->deadline <= task->deadline) {
            prev = next;
            next = next->ptr;
        }
        if (next == NULL) {
            prev->ptr = tmp;
        } else {
            if (prev) {
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

static ReadyList* runEDF(ReadyList *head, SystemSpecs system, int *time, float *total_energy) {
    TaskObject *task = head->task;
    float energy = ((float)system.CPU_1188 * (float)task->WCET_1188) * 0.001;
    printf("%d %s %d %d %fJ\n", *time, task->task_name, 1188, task->WCET_1188, energy);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;
    *total_energy += energy;
    *time += task->WCET_1188;
    return newHead;
}

// Helper function to calculate energy consumption
float calculate_energy(int frequency, int wcet, SystemSpecs system) {
    switch (frequency) {
        case 1188: return (float)system.CPU_1188 * (float)wcet * 0.001;
        case 918: return (float)system.CPU_918 * (float)wcet * 0.001;
        case 648: return (float)system.CPU_648 * (float)wcet * 0.001;
        case 384: return (float)system.CPU_384 * (float)wcet * 0.001;
        default: return 0.0f;
    }
}

static ReadyList* runEDF_EE(ReadyList *head, SystemSpecs system, int *time, float *total_energy) {
    TaskObject *task = head->task;
    int frequencies[] = {1188, 918, 648, 384};
    int wcets[] = {task->WCET_1188, task->WCET_918, task->WCET_648, task->WCET_384};
    int selectedFrequency = frequencies[0];
    int wcet = wcets[0];
    float minEnergy = calculate_energy(selectedFrequency, wcet, system);

    // Select the most energy-efficient frequency
    for (int i = 1; i < 4; i++) {
        if (wcets[i] <= task->deadline) {
            float energy = calculate_energy(frequencies[i], wcets[i], system);
            if (energy < minEnergy) {
                selectedFrequency = frequencies[i];
                wcet = wcets[i];
                minEnergy = energy;
            }
        }
    }

    printf("%d %s %d %d %fJ (EE)\n", *time, task->task_name, selectedFrequency, wcet, minEnergy);
    task->next_deadline += task->deadline;
    task->ready = 0;
    ReadyList *newHead = head->ptr;
    head->ptr = NULL;
    *total_energy += minEnergy;
    *time += wcet;
    return newHead;
}

static void EDF_schedule(TaskObject tasks[], SystemSpecs system, int num_tasks, int Setting) {
    ReadyList *readylist = NULL;
    int time = 0;
    float total_energy_consumption = 0;
    int total_idle_time = 0;

    while (time < system.system_ex_time) {
        for (int j = 0; j < num_tasks; j++) {
            if (time >= tasks[j].next_deadline && tasks[j].ready == 0) {
                tasks[j].ready = 1;
                readylist = insert(readylist, &tasks[j]);
            }
        }
        if (readylist) {
            if (Setting == EE_SETTING) {
                readylist = runEDF_EE(readylist, system, &time, &total_energy_consumption);
            } else {
                readylist = runEDF(readylist, system, &time, &total_energy_consumption);
            }
        } else {
            int closestDeadline = INT_MAX;
            int index = 0;
            for (int k = 0; k < num_tasks; k++) {
                if (tasks[k].next_deadline - time < closestDeadline) {
                    closestDeadline = tasks[k].next_deadline - time;
                    index = k;
                }
            }
            int idle_time = tasks[index].next_deadline - time;
            total_idle_time += idle_time;
            float energy = (float)idle_time * (float)system.CPU_idle * 0.001;
            printf("%d IDLE IDLE %d %fJ\n", time, idle_time, energy);
            total_energy_consumption += energy;
            time = tasks[index].next_deadline;
        }
    }

    printf("%fJ %f%% %ds\n", total_energy_consumption, (float)total_idle_time / (float)system.system_ex_time, system.system_ex_time);
}

int main(int argc, char **argv) {
    FILE *file;
    file = fopen("input1.txt", "r");

    int num_tasks;
    SystemSpecs system;

    if (file == NULL) {
        printf("Couldn't open file");
        exit(1);
    }

    fscanf(file, "%d %d %d %d %d %d %d", &num_tasks, &system.system_ex_time, &system.CPU_1188, &system.CPU_918, &system.CPU_648, &system.CPU_384, &system.CPU_idle);

    TaskObject taskArray[num_tasks];

    for (int i = 0; i < num_tasks; i++) {
        fscanf(file, "%s %d %d %d %d %d", taskArray[i].task_name, &taskArray[i].deadline, &taskArray[i].WCET_1188, &taskArray[i].WCET_918, &taskArray[i].WCET_648, &taskArray[i].WCET_384);
        taskArray[i].next_deadline = 0;
        taskArray[i].ready = 0;
    }

    fclose(file);

    EDF_schedule(taskArray, system, num_tasks, EE_SETTING); // Use DEFAULT for non-EE scheduling

    return 0;
}
