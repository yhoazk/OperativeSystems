// ConsoleApplication1.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"

#include <stdio.h>
/* Variadic + str concatenation */
// operators # and ## to stringize
//


/* __COUNTER__ Macro is self incrementing at preprocessing time  */
#define TRUE  (1==1)
#define FALSE (!TRUE)

#define TASK_NUMBER (4)

#define PRIORITY_BASED (FALSE)

typedef struct task
{

    int(*pTask)(void);
    size_t prio;
    /* There is no priority, the tasks run one after other */
     task *next;
}task_t;
typedef int(*fptr)();

/* The lower the value, the higher the priority */


#if PRIORITY_BASED == FALSE
#define TASK(fnc, ...)          {&fnc, ##__VA_ARGS__, &TASKS [ __COUNTER__+1]}
#else
#define TASK(fnc, ...)          {&fnc, ##__VA_ARGS__, &TASKS[TASK_NUMBER]}
#endif
// the ## eliminate the comma

/* Init Tasks */
int print_1(void);
int print_2(void);
int print_3(void);
int print_4(void);
int scheduler(void);
task_t TASKS[TASK_NUMBER + 1] = {
    /*DECLARE YOUR COMMA SEPARATED TASKS HERE  */
    // TEMPLATE:
    // TASK(<fnc_name>, [<prio>])
    TASK(print_1, 2),
    TASK(print_2, 1),
    TASK(print_3, 0),
    TASK(print_4, 0),
    // DO NOT MODIFY THE NEXT LINE
    { scheduler, 0, &TASKS[0] }
};


int scheduler(void)
{
    return 1;

}


int print_1(void)
{
    printf("1");
    return 1;
}


int print_2(void)
{
    printf("2");
    return 1;
}


int print_4(void)
{
    printf("4");
    return 1;
}

int print_3(void)
{
    printf("3");
    return 1;
}




int main(void)
{
    task_t* t;
    int i = 0;
    t = &TASKS[0];
    for (;;) {
        t->pTask();
        t = t->next;

    }
}

