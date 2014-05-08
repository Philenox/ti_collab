#include "io430.h"

void uart_setup(void)
{
  P3DIR |= (1<<3); //TX Set as output
  P3SEL |= (1<<3) | (1<<4);  // enables UART on P3.3 and 3.4
  UCA0CTL1 |= UCSWRST; //WRST
  UCA0CTL1 |= UCSSEL1; //set clock
  UCA0BR0 |= 833 & 0xff; //set the baud rate
  UCA0BR1 |= 833 >> 8; //baud rate continued
  UCA0IE |= UCTXIE; //enable tx interrupt
  UCA0CTL1 &= ~(UCSWRST); //disable reset
}

void uart_putc(unsigned char data)
{
  UCA0TXBUF = data;
  while(UCA0STAT & UCBUSY)
  {
  }
  //LPM0;
}

/*
#pragma vector = USCI_A0_VECTOR
__interrupt void A0_interrupt(void) {
  __bic_SR_register_on_exit(LPM0_bits);  // Clear LPM0 mode
}
*/
