//  MSP430F552x Demo - ADC12, Sample A10 Temp and Convert to oC and oF
//
//  Description: A single sample is made on A10 with reference to internal
//  1.5V Vref. Software sets ADC12SC to start sample and conversion - ADC12SC
//  automatically cleared at EOC. ADC12 internal oscillator times sample
//  and conversion. In Mainloop MSP430 waits in LPM4 to save power until
//  ADC10 conversion complete, ADC12_ISR will force exit from any LPMx in
//  Mainloop on reti.
//  ACLK = n/a, MCLK = SMCLK = default DCO ~ 1.045MHz, ADC12CLK = ADC12OSC
//
//  Uncalibrated temperature measured from device to devive will vary do to
//  slope and offset variance from device to device - please see datasheet.
//
//  NOTE:REFMSTR bit in REFCTL0 regsiter is reset to allow the ADC12_A reference
//    control regsiters handle the reference setting. Upon resetting the REFMSTR
//    bit, all the settings in REFCTL are 'dont care' and the legacy ADC12  
//    control bits (ADC12REFON, ADC12REF2_5, ADC12TCOFF and ADC12REFOUT) control 
//    the reference system.

#include  "msp430x552x.h"

int i;
long temp;
volatile long IntDegF;
volatile long IntDegC;

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  
    UCSCTL0 |= (31 <<8); // set DCO to 31
  UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
  UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
  UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
  UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
  
  P2DIR |= 1<<2;
  REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control to 
                                            // ADC12_A ref control registers  
  ADC12CTL0 = ADC12SHT0_8 + ADC12REFON + ADC12ON;
                                            // Internal ref = 1.5V
  ADC12CTL1 = ADC12SHP;                     // enable sample timer
  ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense i/p
  ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO
  for(i=0;i<30;i++)                         // Delay to allow Ref to settle
  ADC12CTL0 |= ADC12ENC;
  long int a = 2438;
  long int b = 410;
  long int c = 4096;
  long int d = 2264;
  long int e = 738;
  
  while(1)
  {
    ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start

    __bis_SR_register(LPM4_bits + GIE);     // LPM0 with interrupts enabled
    __no_operation();

    // Temperature in Celsius
    // ((A10/4096*1500mV) - 894mV)*(1/3.66mV) = (A10/4096*410) - 244
    // = (A10 - 2438) * (410 / 4096)
    IntDegC = ((temp - a) * b) / c;

    // Temperature in Fahrenheit
    // ((A10/4096*1500mV) - 829mV)*(1/2.033mV) = (A10/4096*738) - 408
    // = (A10 - 2264) * (738 / 4096)
    IntDegF = ((temp - d) * e) / c;
    __no_operation();                       // SET BREAKPOINT HERE
    if(IntDegC <= 10)
    {
   P2OUT |= 1<<2;
   __delay_cycles(400000);
   P2OUT &= ~(1<<2);
   __delay_cycles(16000000);
    }
    if(IntDegC > 34)
    {
   P2OUT |= 1<<2;
   __delay_cycles(400000);
   P2OUT &= ~(1<<2);
   __delay_cycles(400000);
   P2OUT |= 1<<2;
   __delay_cycles(400000);
   P2OUT &= ~(1<<2);
   __delay_cycles(16000000);
    }
  }
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    temp = ADC12MEM0;                       // Move temp, IFG is cleared
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