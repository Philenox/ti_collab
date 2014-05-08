#include <msp430.h>
#include "uart.h"
#include "adc.h"

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  
  long data;
  int byte1, byte2;
  
  UCSCTL0 |= (31 <<8); // set DCO to 31
  UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
  UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
  UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
  UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
  
  uart_setup();
  adc_setup();
  
  while(1)
  {
      P2DIR |= 1<<2;
      data = adc_sample(1);
      byte1 = data & 0x00ff;
      byte2 = (data >> 8);
      uart_putc(0x00);
      uart_putc(byte2);
      uart_putc(byte1);
      P2OUT ^= 1<<2;
      __delay_cycles(8000000); //delay 1 million cycles
  }
  return 0;
}
