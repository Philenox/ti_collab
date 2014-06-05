#include <msp430.h>
#include <stdint.h>

int i;
void adc_setup()
{
  REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control to 
  ADC12CTL0 |= ADC12SHT0_8 + ADC12REFON + ADC12ON; // ADC12_A ref control registers  
  ADC12CTL1 |= ADC12SHP;                     // enable sample timer
  ADC12MCTL0 |= ADC12SREF_0;  //Internal ref = AVCC & AVSS
  P6DIR = 0x00; //set ADC pins as input
  P6SEL = 0xff; //set ADC pins as ADC
  P1DIR = 0xFF; //Set GPIO pins as output for trigger
  __delay_cycles(30);                        // Delay to allow Ref to settle
  ADC12CTL0 |= ADC12ENC;
}

long adc_sample(uint8_t a)
{
  ADC12CTL0 &= ~(ADC12ENC);
  switch(a)
  {
  case 0:
    P1OUT |= 1<<0;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<0);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_0;  //ADC i/p ch A0
    break;
  case 1:
    P1OUT |= 1<<1;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<1);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_1;  //ADC i/p ch A1
    break;
  case 2:
    P1OUT |= 1<<2;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<2);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_2;  //ADC i/p ch A2
    break;
  case 3:
    P1OUT |= 1<<3;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<3);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_3;  //ADC i/p ch A3
    break;
  case 4:
    P1OUT |= 1<<4;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<4);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_4;  //ADC i/p ch A4
    break;
  case 5:
    P1OUT |= 1<<5;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<5);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_5;  //ADC i/p ch A5
    break;
  case 6:
    P1OUT |= 1<<6;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<6);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_6;  //ADC i/p ch A6
    break;
  case 7:
    P1OUT |= 1<<7;
    __delay_cycles(400); //delay for 50uS
    P1OUT &= ~(1<<7);
    __delay_cycles(480000); //delay for 60mS
    ADC12MCTL0 |= ADC12INCH_7;  //ADC i/p ch A7
    break;
  }
  
  ADC12CTL0 |= ADC12ENC;
  ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start
  while (!(ADC12IFG & ADC12IFG0))      // wait till done sampling
  {
  }
  ADC12CTL0 &= ~ADC12ENC;
  ADC12MCTL0 = 0x00; //clear
  return ADC12MEM0;
}