#ifndef _ischeduler_H
#define _ischeduler_H

#ifdef WTIMER
#include "libmf\wtimer.h"
#define scheduler_desc wtimer_desc
#define scheduler_desc_handler_t wtimer_desc_handler_t
#define scheduler_HAL_st wtimer_HAL_st
#else
#include "watimer\watimer.h"
#define ABSOLUTE    0
#define RELATIVE    1
#define scheduler_desc watimer_callback_st
#define scheduler_desc_handler_t watimer_callback_func
#define scheduler_HAL_st watimer_HAL_st
#endif



typedef struct 
{
  void (* __scheduler_run_callbacks)(void);
  void (* __scheduler_add_task)(struct scheduler_desc *, scheduler_desc_handler_t, uint8_t, uint32_t);
  void (* __scheduler_remove_task)(struct scheduler_desc *);
  uint32_t (* __scheduler_curr_time)(void);
}ischeduler_st;

#endif //_ischeduler_H