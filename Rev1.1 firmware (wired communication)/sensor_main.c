#include <msp430.h>
#include "stdint.h"


#define MAX_RANGE 5 //in meters
#define MAX_CYCLES 33330 //MAX_RANGE*2*3333 // in cycles
#define DIVISOR 2 // 65535 / MAX_CYCLES

///////Don't change anything beyond this point
void spi_init();
char spi_transfer(char);
void pin_init (void);
void set_timer(unsigned int);
uint8_t current_sensor;

void main()
{
  pin_init();
  spi_init();
  WDTCTL = WDTPW + WDTHOLD; 
  current_sensor = 0;
  __bis_SR_register(GIE);  // Enable interrupts (set GIE in SR)
  
  P2OUT &= ~(1<<2);
  
  while(1)
  {
    set_timer(MAX_CYCLES);
    LPM0;
  }
}

void set_timer(unsigned int delay_time)
{
  TA0R = 0x00;
  TA0CCR0 = delay_time;               
  TA0CCTL0 |= CCIE;                   // Compare-mode interrupt
  P1IFG &= ~(0xff);                      // clear all previous interruptss
  P1IE |= (1<< current_sensor);        // enable interrupt for sensor
  P6OUT |= (1<< current_sensor);
  TA0CTL = TASSEL_2 + MC_1;           // TACLK = SMCLK, Up mode
  __delay_cycles(10);                 // 10us is enough to be picked up
  P6OUT &= ~(1<< current_sensor);
}

void pin_init (void)
{
  //LED pin set to output
  P2DIR |= (1<<2);
  
  // Configure ports on MSP430 device for USCI_A1
  P4DIR |= BIT0 | BIT4;
  P4DIR &= ~(BIT4);
  P4SEL |= BIT0 | BIT4 | BIT5;
  
  //Set all of PORT6 to outputs, for triggering signal
  P6DIR |= 0xff;
  
  //Set all of PORT1 as Interrupts
  P1DIR &= ~(0xff);// PORT1 is input
  P1OUT &= ~0xff;   // Pull-up resistor disable
  P1REN |= 0xff;
  P1IES |= 0x00;   // Trigger on rising-edge
  P1IFG &= ~0xff;  // Clear any outstanding IRQ
  P1IE &= ~(0xff); // Disable all interrupts
}

void spi_init()
{
	// Configure ports on MSP430 device for USCI_A1
        P4DIR |= BIT0 | BIT4;
        P4DIR &= ~(BIT5);
	P4SEL |= BIT0 | BIT4 | BIT5;
	// USCI-A specific SPI setup 
	UCA1CTL1 |= UCSWRST;
	UCA1CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;  // SPI mode 0, master
	UCA1BR0 = 0x01;  // SPI clocked at same speed as SMCLK
	UCA1BR1 = 0x00;
	UCA1CTL1 = UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
}

char spi_transfer(char inb)
{
	//UCA1IE = UCRXIE;
        UCA1TXBUF = inb;
	do {
	//	LPM0;
	} while (UCA1STAT & UCBUSY);
	return UCA1RXBUF;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR(void)
{
  uint8_t temp;
  temp = current_sensor;
  TA0CTL &= ~(MC_1); //stop
  P1IE &= ~(1<< current_sensor);        // disable interrupt for sensor
  //Take measurement
  temp =  ((0x07&current_sensor) << 5);
  spi_transfer(temp);
  //current_sensor++;
  __bic_SR_register_on_exit(LPM0_bits);
}

//Interrupt pin
#pragma vector = PORT1_VECTOR
__interrupt void P1_interrupt (void) {
  uint16_t data;
  TA0CTL &= ~(MC_1); //stop
  P1IE &= ~(1<< current_sensor);        // disable interrupt for sensor
  //Take measurement
  data = (TA0R / DIVISOR) & (0x1F); // grab the data, scale it to 32 bits
  data = 0x1F - data; //Inverse it, 
  
  if(TA0R >  650)
    P2OUT |= (1<<2);
  else
    P2OUT &= ~(1<<2);
  data =  ((data) | (0x07 & current_sensor) << 5);
  spi_transfer(data);
  //current_sensor++;
  __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_TX(void) {
	UCA1IE &= ~UCTXIE;
	__bic_SR_register_on_exit(LPM0_bits);  // Clear LPM0 mode
}

