#include <string.h>
#include "watimer\watimer.h"

volatile uint32_t watimer_time;

volatile uint8_t  callbacks_num;

struct watimer_callback_st* watimer_callbacks[MAXCALLBACKS];

watimer_HAL_st *watimer_hal = 0;

void watimer_set_HAL(watimer_HAL_st *ptr)
{
  watimer_hal = ptr;
}

uint32_t watimer_update_time()
{
    static uint32_t old_watimer_time = 0;
    
   
    watimer_time &= 0xffff0000;
    watimer_time += watimer_hal->__cnt_get(0);
    
    if(old_watimer_time > watimer_time) watimer_time += 0x10000;
       
     //   if((old_watimer_time == watimer_time) &&(watimer_time == 3085))
     // watimer_time++;
    
    return (old_watimer_time = watimer_time);
}


static void watimer_update_callbacks()
{
    watimer_update_time();   
    for(uint8_t i = 0; i < callbacks_num; i++)
    {
    	if(watimer_callbacks[i])
    	{
     		if((watimer_callbacks[i]->timeout == 0 )&&(watimer_time - watimer_callbacks[i]->timer < 0x80000000))
    		{
                        if(watimer_callbacks[i]->level == RUN_CONTINUOSLY_RELATIVE) watimer_callbacks[i]->timer = watimer_time + watimer_callbacks[i]->period; 
                        watimer_callbacks[i]->timeout = 1;
    		}
    	}
    }
}


static _Bool watimer_configure_next_irq_time()
{
  
  uint32_t irq_time = 0xffffffff;
  _Bool pending = 0;
  _Bool timeout = 0;
  watimer_update_callbacks();
  for(uint8_t i = 0; i < callbacks_num; i++)
  {
    if(watimer_callbacks[i] && watimer_callbacks[i]->timeout)
    {
      timeout = 1;
      continue;
    }
    
    if(watimer_callbacks[i])
    {
       pending = 1;
       if(watimer_callbacks[i]->timer < irq_time) irq_time = watimer_callbacks[i]->timer;
    }
  }
  
  if(pending)
  {
    watimer_hal->__cc_irq_enable(0);
    watimer_update_time();
   //   if(watimer_time == 3085)
   //   watimer_time++;
    //if(((irq_time&0xffff) - ((watimer_time + MILLISECONDS(5))&0xffff)) < 0x8000)
    if(irq_time > (watimer_time + MILLISECONDS(5)))
    {
      watimer_hal->__cc_set(0, irq_time&0xffff);
    }
    else 
    {
      if(!watimer_hal->__check_cc_irq(0)) 
      {
        watimer_hal->__cc_set(0, watimer_hal->__cnt_get(0) + MILLISECONDS(5));
      }
      else
      {
        watimer_hal->__cc_irq_disable(0);
        watimer_hal->__cc_irq_enable(0);
        watimer_hal->__cc_set(0, watimer_hal->__cnt_get(1) + MILLISECONDS(5));
      }
    }
  }
  else
  {
    //timeout = 1;
   // watimer_hal->__cc_set(0, watimer_hal->__cnt_get(1) + SECONDS(10));
  }


  
  
  return timeout;
}


void watimer_init(void)
{
  if(watimer_hal == 0) while(1); //HAL struct must be configured before library usage 
  watimer_time = watimer_hal->__cnt_get(0);
  callbacks_num = 0;
  memset(((uint8_t*)(&watimer_callbacks[0])), 0, sizeof(watimer_callbacks));  
}



void watimer_irq(void)
{   
    //watimer_configure_next_irq_time();
    watimer_run_callbacks();
}

void watimer_run_callbacks()
{  
       watimer_hal->__global_irq_disable();
        
	watimer_update_callbacks();
        struct watimer_callback_st* tmp;
	for(uint8_t i = 0; i < callbacks_num; i++)
	{
		if(watimer_callbacks[i])
	   	{
	   		if(watimer_callbacks[i]->timeout)
	   		{
	   			watimer_callbacks[i]->timeout = 0;
	   			tmp = watimer_callbacks[i];
	   			if(watimer_callbacks[i]->level != RUN_CONTINUOSLY_RELATIVE) 
                                {
                                  watimer_remove_callback(watimer_callbacks[i]);
                                }
	   			
                                watimer_hal->__global_irq_enable();                               
                                tmp->func(tmp);                                
                                watimer_hal->__global_irq_disable();
	   		}
	   	}
	 }
         if(watimer_configure_next_irq_time()) watimer_run_callbacks();
         watimer_hal->__global_irq_enable(); 
}
uint16_t diff;
_Bool watimer_can_sleep()
{
  _Bool soon = 0;
  watimer_hal->__global_irq_disable();
  diff = watimer_hal->__cc_get(0) - watimer_hal->__cnt_get(0);
  if(diff < 3) soon = 1; 
  watimer_hal->__global_irq_enable(); 
  
  return (!soon);
}

void watimer_add_callback(struct watimer_callback_st* desc, watimer_callback_func cb, watimer_run_mode_en run_level, uint32_t period)
{
        watimer_hal->__global_irq_disable();	
        uint8_t p;
	for(p = 0; p < callbacks_num; p++) if(watimer_callbacks[p] == desc)
	{
		goto init;
	}
	for(p = 0; p < callbacks_num; p++) if(!watimer_callbacks[p]) break;
	if((p == callbacks_num))
	{
		if (p >= MAXCALLBACKS) 
                {
                  watimer_hal->__global_irq_enable(); 
                  while(1); //halt on callbacks buffer overflow
                }
		callbacks_num++;
	}
        watimer_callbacks[p] = desc;
        if(cb) watimer_callbacks[p]->func = cb;
init:
	watimer_callbacks[p]->period = period;
	watimer_callbacks[p]->level = run_level;
	watimer_callbacks[p]->timer = watimer_update_time() + period;
	watimer_callbacks[p]->timeout = 0;            
        watimer_configure_next_irq_time();
        watimer_hal->__global_irq_enable(); 
	
}

void watimer_remove_callback(struct watimer_callback_st* desc)
{
        watimer_hal->__global_irq_disable();	
        
	for(uint8_t p = 0; p < callbacks_num; p++)
		if(watimer_callbacks[p] == desc)
		{
			watimer_callbacks[p] = 0;
			if(p == (callbacks_num - 1)) callbacks_num--;
			break;
		}
        watimer_configure_next_irq_time();
        watimer_hal->__global_irq_enable(); 
}

_Bool watimer_check_callback(struct watimer_callback_st* desc)
{
        watimer_hal->__global_irq_disable();	
        
	uint8_t p;
	for(p = 0; p < callbacks_num; p++)
		if(watimer_callbacks[p] == desc) break;
	if(p == callbacks_num) p = 0;
	else p = 1;       
        watimer_hal->__global_irq_enable(); 
	return p;
}
