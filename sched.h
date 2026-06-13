#ifndef SCHED_H
#define SCHED_H

#include "irq.h" /* For struct cpu_regs */

struct vcpu_task {
    int id;
    struct cpu_regs context;
    struct vcpu_task *next;
};

void sched_init(void);
void sched_add_task(struct vcpu_task *task);
void schedule(void);
void context_switch(struct cpu_regs *prev, struct cpu_regs *next);

#endif
