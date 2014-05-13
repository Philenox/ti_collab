#include <msp430.h>
#include <stdint.h>

int i;
void adc_setup()
{
  P6DIR &= ~(1<<0);
  P6SEL |= 1<<0;
  REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control to 
  ADC12CTL0 |= ADC12SHT0_8 + ADC12REFON + ADC12ON; // ADC12_A ref control registers  
  ADC12CTL1 |= ADC12SHP;                     // enable sample timer
  ADC12MCTL0 |= ADC12SREF_0;  //Internal ref = AVCC & AVSS
  ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO
  for(i=0;i<30;i++)                         // Delay to allow Ref to settle
  ADC12CTL0 |= ADC12ENC;
}

long adc_sample(uint8_t a)
{
  ADC12CTL0 &= ~(ADC12ENC);
  switch(a)
  {
  case 0:
    ADC12MCTL0 |= ADC12INCH_0;  //ADC i/p ch A0
    P6DIR &= ~(1<<0);
    P6SEL |= 1<<0;
    break;
  case 1:
    ADC12MCTL0 |= ADC12INCH_1;  //ADC i/p ch A1
    P6DIR &= ~(1<<1);
    P6SEL |= 1<<1;
    break;
  case 2:
    ADC12MCTL0 |= ADC12INCH_2;  //ADC i/p ch A2
    P6DIR &= ~(1<<2);
    P6SEL |= 1<<2;
    break;
  case 3:
    ADC12MCTL0 |= ADC12INCH_3;  //ADC i/p ch A3
    P6DIR &= ~(1<<3);
    P6SEL |= 1<<3;
    break;
  case 4:
    ADC12MCTL0 |= ADC12INCH_4;  //ADC i/p ch A4
    P6DIR &= ~(1<<4);
    P6SEL |= 1<<4;
    break;
  case 5:
    ADC12MCTL0 |= ADC12INCH_5;  //ADC i/p ch A5
    P6DIR &= ~(1<<5);
    P6SEL |= 1<<5;
    break;
  case 6:
    ADC12MCTL0 |= ADC12INCH_6;  //ADC i/p ch A6
    P6DIR &= ~(1<<6);
    P6SEL |= 1<<6;
    break;
  case 7:
    ADC12MCTL0 |= ADC12INCH_7;  //ADC i/p ch A7
    P6DIR &= ~(1<<7);
    P6SEL |= 1<<7;
    break;
  }
  
  ADC12CTL0 |= ADC12ENC;
  ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start
  while (~(ADC12MCTL0 & ADC12EOS))      // wait till done sampling
  return ADC12MEM0;
}
