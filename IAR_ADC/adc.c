#include <msp430.h>

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

long adc_sample(int a)
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

/*
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    data = ADC12MEM0;                       // Move temp, IFG is cleared
    __bic_SR_register_on_exit(LPM4_bits);   // Exit active CPU
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}
*/