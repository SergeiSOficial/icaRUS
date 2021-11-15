#ifndef _scheduler_H
#define _scheduler_H

#include "ischeduler.h"


ischeduler_st* scheduler_init(scheduler_HAL_st *ptr);

void scheduler_irq();

void scheduler_run_callbacks();
void scheduler_add_task(struct scheduler_desc *desc, scheduler_desc_handler_t handler, uint8_t relative, uint32_t time);
void scheduler_remove_task(struct scheduler_desc *desc);
uint32_t scheduler_current_time();
_Bool scheduler_can_sleep();

#endif //