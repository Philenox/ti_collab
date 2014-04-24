
#include "io430.h"

void i2c_setup ( void )
{
  P4DIR = 1<<1 | 1<<2;
  P4SEL = 1<<1 | 1<<2;
  UCB1CTL1 = 1; //SW reset
  UCB1CTL0 = 1<<3 | 1<<2 | 1<<1 | 1; //Master mode, I2C mode, Synchronous 
  UCB1CTL1 |= 1<<7; //Clock source, Transmitter
  UCB1BR0 = 0x01; //baud rate
  UCB1BR1 = 0x00;
  UCB1CTL1 &= ~1;
}

void send_i2c ( char data )
{
  UCB1CTL1 |= UCTR | UCTXSTT;
  UCB1TXBUF = data;
  //UCB1CTL1 |= UCTXSTP;
}


int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  
  P2DIR |= 1<<2;
  //P4DIR |= 1<<2;
  while(1){
    P2OUT ^= 1<<2;
    //P4OUT ^= 1<<2;
    send_i2c('a');
    __delay_cycles(1600000);
  }
}
