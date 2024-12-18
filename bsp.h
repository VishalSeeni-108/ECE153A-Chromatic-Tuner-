/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h



/* bsp functions ..........................................................*/

void BSP_init(void);
void ISR_gpio(void);
void ISR_timer(void);

void printDebugLog(void);

void button_handler(void *CallbackRef);
void timer_handler();

static int currNote;

#define BSP_showState(prio_, state_) ((void)0)


#endif                                                             /* bsp_h */


