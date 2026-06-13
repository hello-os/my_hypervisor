#include "sched.h"

static struct vcpu_task *ready_list = 0;
static struct vcpu_task *current_task = 0;

void sched_init(void)
{
    ready_list = 0;
    current_task = 0;
}

void sched_add_task(struct vcpu_task *task)
{
    if (!ready_list) {
        ready_list = task;
        task->next = 0;
    } else {
        struct vcpu_task *temp = ready_list;
        while (temp->next) temp = temp->next;
        temp->next = task;
        task->next = 0;
    }
}

void schedule(void)
{
    if (!ready_list) return;

    struct vcpu_task *next = ready_list;
    
    /* Simple round-robin */
    ready_list = next->next;
    
    if (current_task) {
        struct vcpu_task *temp = ready_list;
        if (!temp) {
            ready_list = current_task;
            current_task->next = 0;
        } else {
            while (temp->next) temp = temp->next;
            temp->next = current_task;
            current_task->next = 0;
        }
    }

    if (next != current_task) {
        struct vcpu_task *prev = current_task;
        current_task = next;
        
        if (prev) {
            context_switch(&prev->context, &next->context);
        } else {
            /* First run: initialize SP_EL2 manually if needed */
        }
    }
}
