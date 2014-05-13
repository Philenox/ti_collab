#include <msp430.h>

void uart_setup(void)
{
  UCA0CTL1 |= UCSWRST; //WRST
  
  P3DIR |= (1<<3); //TX Set as output
  P3SEL |= (1<<3) | (1<<4);  // enables UART on P3.3 and 3.4
  UCA0CTL1 |= UCSSEL1; //set clock
  UCA0BR0 |= 833 & 0xff; //set the baud rate
  UCA0BR1 |= 833 >> 8; //baud rate continued
  UCA0MCTL |= UCBRS1; //this is used for buad rate selection
  UCA0CTL1 &= ~(UCSWRST); //disable reset
  
//  UCA0IE |= UCTXIE | UCRXIE; //enable tx + rx interrupt
}

void uart_putc(unsigned char data)
{
  UCA0TXBUF = data;
//  UCA0IE |=  UCTXIE;
//  UCA0IFG |= UCTXIFG;
//  LPM0;
}

/*
#pragma vector = USCI_A0_VECTOR
__interrupt void A0_interrupt(void) {
  __bic_SR_register_on_exit(LPM0_bits);  // Clear LPM0 mode
}
*/
