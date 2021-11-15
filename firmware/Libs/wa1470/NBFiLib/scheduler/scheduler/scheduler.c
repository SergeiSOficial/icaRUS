#include "nbfi_defines.h"
#include "scheduler.h"

ischeduler_st scheduler_inst;

void scheduler_irq()
{
  #ifdef WTIMER
  wtimer_cc0_irq();
  #else   
  watimer_irq();
  #endif
}

void scheduler_run_callbacks()
{
  #ifdef WTIMER
  wtimer_runcallbacks();
  #else
  watimer_run_callbacks();
  #endif
}


_Bool scheduler_can_sleep()
{
  #ifdef WTIMER
  return 1;
  #else
  return watimer_can_sleep();
  #endif
}

uint32_t scheduler_curr_time()
{
  #ifdef WTIMER
  return wtimer_state[0].time.cur;
  #else
  return watimer_update_time();
  #endif
}

void scheduler_add_task(struct scheduler_desc *desc, scheduler_desc_handler_t handler, uint8_t relative, uint32_t time)
{
  #ifdef WTIMER
  ScheduleTask(desc, handler, relative, time);
  #else
  watimer_add_callback(desc, handler, (watimer_run_mode_en)relative, time);
  #endif
}

void scheduler_remove_task(struct scheduler_desc *desc)
{
  #ifdef WTIMER
  wtimer0_remove(desc);
  #else
  watimer_remove_callback(desc);
  #endif
}

uint32_t scheduler_current_time()
{
  #ifdef WTIMER
  return wtimer_state[0].time.cur;
  #else
  return watimer_time;
  #endif
}

ischeduler_st* scheduler_init(scheduler_HAL_st *ptr)
{
  #ifdef WTIMER
  wtimer_set_HAL(ptr);
  wtimer_init();
  #else 
  watimer_set_HAL(ptr);
  watimer_init();
  #endif
  
  scheduler_inst.__scheduler_add_task = &scheduler_add_task;
  scheduler_inst.__scheduler_run_callbacks = &scheduler_run_callbacks;
  scheduler_inst.__scheduler_remove_task = &scheduler_remove_task;
  scheduler_inst.__scheduler_curr_time = &scheduler_curr_time;
  return &scheduler_inst;
}