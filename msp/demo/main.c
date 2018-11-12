#include "msp430.h"
volatile unsigned int i; // volatile to prevent optimization

void main(void)
{
   WDTCTL = WDTPW + WDTHOLD; // stop watchdog timer
   P1DIR |= 0X01; // set P1.0 to output direction
   for(;;) {
       P1OUT ^= 0x01; // toogle P1.0 exclusive-OR
       i = 50000; // delay
       do (i--);
       while (i != 0);
   }
}

