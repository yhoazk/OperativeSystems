#include <stdio.h>
/* Variadic + str concatenation */
// operators # and ## to stringize
//


/* __COUNTER__ Macro is self incrementing at preprocessing time  */
#define TRUE  (1==1)
#define FALSE (!TRUE)

#define TASK_NUMBER (3)

#define PRIORITY_BASED (TRUE)

typedef struct task_t 
{
  
    int (*pTask)(void);
    size_t prio;
#if PRIORITY_BASED == FALSE
    /* There is no priority, the tasks run one after other */
    task_t &next;
#else
    
#endif
};

/* The lower the value, the higher the priority */


#if PRIORITY_BASED == FALSE
    #define TASK(fnc, ...)          {&fnc, ##__VA_ARGS__, &TASKS [ __COUNTER__+1]}
#else
    #define TASK(fnc, ...)          {&fnc, ##__VA_ARGS__, &TASKS[TASK_NUMBER]}
#endif
// the ## eliminate the comma

/* Init Tasks */

task_t TASKS[TASK_NUMBER+1] = {
    /*DECLARE YOUR COMMA SEPARATED TASKS HERE  */
    // TEMPLATE:
    // TASK(<fnc_name>, [<prio>])
    TASK(print_1, 2),
    TASK(print_2, 1),
    TASK(print_3, 0),
    // DO NOT MODIFY THE NEXT LINE
    { scheduler, 0, TASKS}
};


int scheduler (void)
{

}


int print_1 (void)
{
    printf("1");
}


int print_2 (void)
{
    printf("2");
}



int print_3 (void)
{
    printf("3");
}





int main(void)
{
    (*TASK[0])();
}

