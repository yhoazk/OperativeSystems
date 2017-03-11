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

#define TASK_NUMBER (4)

#define PRIORITY_BASED (FALSE)

typedef struct task
{

    int(*pTask)(void);
#if PRIORITY_BASED == TRUE
    size_t prio;
    size_t state = 0;
#endif
    /* There is no priority, the tasks run one after other */
    struct task *next;
}task_t;
typedef int(*fptr)();


#if PRIORITY_BASED == TRUE
#define TASK(fnc, ...)          {&fnc, ##__VA_ARGS__, &TASKS [ __COUNTER__+1]}
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
    TASK(print_1, 2),
    TASK(print_2, 1),
    TASK(print_3, 0),
    TASK(print_4, 3),
    // Add the first element
	{null_task,  TASKS}
};


task_t* scheduler(task_t* current)
{
	task_t* next;
#if PRIORITY_BASED == TRUE
	size_t i = 0;
	/* Look for ready tasks with higher priority */
	for(; i<TASK_NUMBER; i++)
	{
		if(current->prio > TASKS[i].prio)
		{
			next = TASKS[i]->next;
		}
	}
#else
	next = current->next;
#endif

    return next;

}


int print_1(void)
{
	PRINTF("1");
    return 1;
}


int print_2(void)
{
	PRINTF("2");
    return 1;
}


int print_4(void)
{
	PRINTF("4");
    return 1;
}

int print_3(void)
{
	PRINTF("3");
    return 1;
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

    PRINTF("Start.\r\n");

    while (1)
    {

       (void) t->pTask();
       //t->ready
       t = scheduler(t);

    }
}
