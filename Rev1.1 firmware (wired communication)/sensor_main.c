#include <msp430.h>
#include "stdint.h"


#define MAX_RANGE 5 //in meters
#define MAX_CYCLES 33330 //MAX_RANGE*2 (round trip distance) * 1e6 cycles/second / (300m/s)
#define DIVISOR 1075 //MAX_CYCLES / (31) (this is used for scaling data from 16bit integer that is the raw nuber of cycles to a 5 bit number)

void spi_init();
char spi_transfer(char);
void pin_init (void);
void set_timer(unsigned int);


//global varaible that stores which sensor ultrasonic sensor we are currently measuring
uint8_t current_sensor;

void main()
{
  pin_init();                 //setup the pins
  spi_init();                 //setup the SPI
  WDTCTL = WDTPW + WDTHOLD;   //turn off that damn watchdog!!!!!
  current_sensor = 0; 
  __bis_SR_register(GIE);     // Enable interrupts (set GIE in SR)
  
  P2OUT &= ~(1<<2);
  
  while(1)
  {
    set_timer(MAX_CYCLES); //start the timer, count to the max cycles (max range) and stop after that
    LPM0; // LPM0 make it so the code won't advance, until the LPM0 is cleared
  }
}

void set_timer(unsigned int delay_time)
{
  TA0R = 0x00;                        //set the timer to 0
  TA0CCR0 = delay_time;               //count to the delay_time before interrupting               
  TA0CCTL0 |= CCIE;                   // Compare-mode interrupt
  P1IFG &= ~(0xff);                   // clear all previous interruptss
  P1IE |= (1<< current_sensor);       // enable interrupt for sensor
  P6OUT |= (1<< current_sensor);      // trigger the sensor
  TA0CTL = TASSEL_2 + MC_1;           // TACLK = SMCLK, Up mode (RELEASE THE TIMERRRRR!)
  __delay_cycles(10);                 // delay 10us so that the sensor will detect the trigger
  P6OUT &= ~(1<< current_sensor);     // turn the trigger off
}

void pin_init (void)
{
  //LED pin set to output
  P2DIR |= (1<<2);
  
  // Configure ports on MSP430 device for USCI_A1 (SPI)
  P4DIR |= BIT0 | BIT4;
  P4DIR &= ~(BIT4);
  P4SEL |= BIT0 | BIT4 | BIT5;
  
  //Set all of PORT6 to outputs, for triggering signal
  P6DIR |= 0xff;
  
  //Set all of PORT1 as Interrupts
  P1DIR &= ~(0xff);  // PORT1 is input
  P1OUT &= ~0xff;    // Pull-up not selected
  P1REN |= 0xff;     // Pull-up resistor disable
  P1IES |= 0x00;     // Trigger on rising-edge
  P1IFG &= ~0xff;    // Clear any outstanding IRQ
  P1IE &= ~(0xff);   // Disable all interrupts
}

void spi_init()
{
	// Configure ports on MSP430 device for USCI_A1
  P4DIR |= BIT0 | BIT4;
  P4DIR &= ~(BIT5);
	P4SEL |= BIT0 | BIT4 | BIT5;
	// USCI-A specific SPI setup 
	UCA1CTL1 |= UCSWRST; //enable reset
	UCA1CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;  // SPI mode 0, master
	UCA1BR0 = 0x01;  // SPI clocked at same speed as SMCLK
	UCA1BR1 = 0x00;
	UCA1CTL1 = UCSSEL_2;  // Clock = SMCLK, clear UCSWRST and enables USCI_B module.
}

char spi_transfer(char inb)
{
  //load that stuff in the buffer
  UCA1TXBUF = inb;
	do {
	} while (UCA1STAT & UCBUSY); //wait until the SPI module isn't busy, then we may proceed
	return UCA1RXBUF; //this isn't actually used, becase we are doing uni-directional communication
}


//this is the interrupt for the timer, when this triggers, it means that max time has elapsed
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR(void)
{
  uint8_t temp;
  TA0CTL &= ~(MC_1);                      // stop the timer (it should alread be stopped but just making sure)
  P1IE &= ~(1<< current_sensor);          // disable interrupt for sensor (that way it doesn't trigger on a different sensor)
  P1IFG = 0;                              // clear all the interrupt flags, just in case it triggers after this interrupt got called
  temp =  ((0x07&current_sensor) << 5);   // take measurement
  spi_transfer(temp);                     // send off the data!!!
  current_sensor++;                       // move on to the next sensor
  __bic_SR_register_on_exit(LPM0_bits);   // disable the LPM0 (this will make the main loop run again)
}

//Interrupt pin, this is triggered when the 
#pragma vector = PORT1_VECTOR
__interrupt void P1_interrupt (void) {
  uint16_t data;
  TA0CTL &= ~(MC_1);                               //stop the timer
  TA0IV = 0x00;                                    //clear all timer interrupt flags in case it triggered in this ISR
  P1IE &= ~(1<< current_sensor);                   // disable interrupt for sensor
  //Take measurement
  data = (TA0R / DIVISOR);                         // grab the data, scale it to 32 bits
  data = 31 - data;                                //Inverse it, that way the further away the object, the smaller the number
  data =  ((data) | (0x07 & current_sensor) << 5); //create the data packtet   [[ 3 bits describe which sensor was measured][ 5 bits of sensor data]]
  spi_transfer(data);                              //send that data off
  current_sensor++;                                //move on to the next sensor
  __bic_SR_register_on_exit(LPM0_bits);            // disable the LPM0 (this will make the main loop run again)
}


//We've got mail (this is not actually used, but it's still in case we need bi-direction communication for some reason)
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_TX(void) {
	UCA1IE &= ~UCTXIE;
	__bic_SR_register_on_exit(LPM0_bits);  // Clear LPM0 mode
}

