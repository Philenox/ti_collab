#include <msp430.h>
#include "stdint.h"

volatile uint8_t count;
volatile uint8_t global_buf[8];
void spi_init();
void pin_init (void);
void set_timer(unsigned int);

void main()
{
  pin_init();
  spi_init();
  WDTCTL = WDTPW + WDTHOLD;
  
  __bis_SR_register(GIE);  // Enable interrupts (set GIE in SR)
  count = 0;
  set_timer(1062); // this corresponds to 5 bits of resolution at 20Hz at 1Mhz

  while(1)
  {
    LPM0;
  }
}

void set_timer(unsigned int delay_time)
{
  TA0CCR0 = delay_time;                    // Delay to allow Ref to settle
  TA0CCTL0 |= CCIE;                          // Compare-mode interrupt
  TA0CTL = TASSEL_2 + MC_1;           // TACLK = SMCLK, Up mode
}

void spi_init()
{
  // USCI-A specific SPI setup 
  UCA1CTL1 |= UCSWRST;
  UCA1CTL0 = UCCKPH | UCMSB | UCMODE_0 | UCSYNC;  // SPI mode 0, slave
  UCA1BR0 = 0x01;  // SPI clocked at same speed as SMCLK
  UCA1BR1 = 0x00;
  UCA1IE |= UCRXIE;
  UCA1CTL1 |= UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
  UCA1CTL1 &= ~UCSWRST;
}

void pin_init (void)
{
  // Configure ports on MSP430 device for USCI_A1
  P4DIR |= BIT0 | BIT4;
  P4DIR &= ~(BIT4);
  P4SEL |= BIT0 | BIT4 | BIT5;
  
  P2DIR |= (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<6); //set PWM pins to output
  P6DIR |= 0xFF; //set all PORT6 to outputs for pwm
  
  //initialize motor intensity to 0
  for(int i = 0; i < 8; i++)
    global_buf[i] = 31;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR(void)
{
  TA0CTL &= ~(MC_1); //stop
  
  uint8_t buf[8];
  for(int i = 0; i < 8 ; i++)
    buf[i] = global_buf[i];
  
  count = count + 1;
  if (count >= 31)
  {
    count = 0;
  }
    
  if(count == 0)
  {
    P2OUT |= (1<<6) | (1<<1) | (1<<2) | (1<<3);
    P6OUT |= (1<<0) | (1<<1) | (1<<2) | (1<<3);
  }
  
  if(count == buf[0])
    P2OUT &= ~(1<<6);
  if(count == buf[1])
    P2OUT &= ~(1<<1);
  if(count == buf[2])
    P2OUT &= ~(1<<2);
  if(count == buf[3])
    P2OUT &= ~(1<<3);
  if(count == buf[4])
    P6OUT &= ~(1<<0);
  if(count == buf[5])
    P6OUT &= ~(1<<1);
  if(count == buf[6])
    P6OUT &= ~(1<<2);
  if(count == buf[7])
    P6OUT &= ~(1<<3);
  
  TA0CTL |= MC_1; //start it up again
}

#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_RX(void) {
    uint8_t data = UCA1RXBUF;
    global_buf[data>>5] = 0x1F & data;
}
