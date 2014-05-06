
#include "io430.h"
#include "uart.h"

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
 
  UCSCTL0 |= (31 <<8); // set DCO to 31
  UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
  UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
  UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
  UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
 
  
  uart_setup();
  
  P2DIR |= (1<<6) + (1<<7);
  while(1)
  {
    P2OUT ^= (1<<6) | (1<<7); //LED toggle
    __delay_cycles(1000000); //delay 1 million cycles
    uart_putc('a');
  }

}
