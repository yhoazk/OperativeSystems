/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define TRUE  (1==1)
#define FALSE (!TRUE)

#define TASK_NEW 		((size_t)0x0)
#define TASK_RDY 	 	((size_t)0x1)
#define TASK_RUN    	((size_t)0x2)
#define TASK_WAIT 		((size_t)0x3)
#define TASK_TERM		((size_t)0x4)

#define TASK_NUMBER (4)
#define PRINT_LIM  (20)
#define PRIORITY_BASED (TRUE)

typedef struct task
{

    int(*pTask)(void);
#if PRIORITY_BASED == TRUE
    size_t state;
    size_t prio;
#endif
    /* There is no priority, the tasks run one after other */
    struct task *next;
}task_t;
typedef int(*fptr)();


#if PRIORITY_BASED == TRUE
#define TASK(fnc, ...)          {&fnc, 0, ##__VA_ARGS__, &TASKS [ __COUNTER__+1]}
#else
#define TASK(fnc, ...)          {&fnc,  &TASKS[__COUNTER__+1]}
#endif
// the ## eliminates the comma

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* Init Tasks */
int print_1(void);
int print_2(void);
int print_3(void);
int print_4(void);
task_t* scheduler(task_t* current);
int null_task(void);
/*******************************************************************************
 * Code
 ******************************************************************************/
task_t TASKS[TASK_NUMBER + 1] = {
    /*DECLARE YOUR COMMA SEPARATED TASKS HERE  */
    // TEMPLATE:
    // TASK(<fnc_name>, [<prio>])
    TASK(print_1, 1),
    TASK(print_2, 4),
    TASK(print_3, 3),
    TASK(print_4, 5),
    // Add the first element
	{null_task, 0, TASKS}
};
size_t current_task = 0;

task_t* scheduler(task_t* current)
{
	task_t* next = current; // in case there is no higher priority
#if PRIORITY_BASED == TRUE
	size_t i = 0;
	size_t curr_prio = 0;
	//if(current->state == TASK_TERM)
	{
		//curr_prio =  0 //as the last task finished restart the priority;
	}
	/* Look for ready tasks with the highest priority which state is new or ready */
	for(; i<TASK_NUMBER; i++)
	{
		if( (TASKS[i].state == TASK_NEW || TASKS[i].state == TASK_RDY || TASKS[i].state == TASK_RUN) )
		{
			if( curr_prio <= TASKS[i].prio)
			{
				curr_prio = TASKS[i].prio;
				next = &TASKS[i];
				TASKS[i].state = TASK_RUN;
				current_task = i;
			}

		}
	}

#else
	next = current->next;
#endif

    return next;

}


int print_1(void)
{
	int state = TASK_RDY;
	static size_t cnt = 0;
	if(cnt++ < PRINT_LIM)
	{
		PRINTF("_1\n");
	}
	else
	{
		state = TASK_TERM;
	}

	return state;
}


int print_2(void)
{
	int state = TASK_RDY;
	static size_t cnt = 0;
	if(cnt++ < PRINT_LIM)
	{
		PRINTF("__2\n");
	}
	else
	{
		state = TASK_TERM;
	}

	return state;
}


int print_4(void)
{
	static size_t cnt = 0;
	int state = TASK_RDY;
	if(cnt++ < PRINT_LIM)
	{
		PRINTF("____4\n");
	}
	else
	{
		state = TASK_TERM;
	}

    return state;
}

int print_3(void)
{
	int state = TASK_RDY;
	static size_t cnt = 0;
	if(cnt++ < PRINT_LIM)
	{
		PRINTF("___3\n");
	}
	else
	{
		state = TASK_TERM;
	}

	return state;
}
int null_task(void)
{

    return 1;
}



void init_tasks(void)
{
	int i = 0;
	for(; i<TASK_NUMBER; i++)
	{
		TASKS[i].state = 0;
	}

}
/*!
 * @brief Main function
 */
int main(void)
{

    task_t* t;
    /* the first function is ran first */
    t = &TASKS[0];
    /* Init board hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    init_tasks();
    PRINTF("Start.\r\n");

    while (1)
    {
    	t = scheduler(t);
    	t->state = t->pTask();

    }
}
