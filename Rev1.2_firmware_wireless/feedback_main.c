 /* Copyright (c) 2012, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <msp430.h>
#include "uart.h"
#include "msprf24.h"
#include "nrf_userconfig_feedback.h"
#include "stdint.h"
   
void radio_setup(void);
void set_timer(unsigned int delay_time);

volatile unsigned int user;
char buf[32];
uint8_t increment;
volatile uint8_t count;             // this is the the global counter that counts for the PWM
char global_buf[32];     // this is the the data for each buzzer

char pwmDbg = 0;

void main(){
  WDTCTL = WDTHOLD | WDTPW;
        
  // Set all clocks to 8Mhz
  UCSCTL0 |= (31 <<8); // set DCO to 31
  UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
  UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
  UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
  UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
  
  /* Set Motor PWMs as outputs
    M1 -> 6.0
    M2 -> 6.3
    M3 -> 6.2
    M4 -> 6.7
    M5 -> 6.6
    M6 -> 2.1
    M7 -> 2.3
    M8 -> 2.2
  */
  P6DIR |= (1<<0) | (1<<2) | (1<<3) | (1<<6) | (1<<7);
  P2DIR |= (1<<1) | (1<<2) | (1<<3);
  
  P2DIR |= (1 << 6); //Set debug LED output  
  
  //initialize motor intensity to 0
  for(int i = 0; i < 8; i++)
    global_buf[i] = 20;        
  
  __bis_SR_register(GIE);   // Enable interrupts (set GIE in SR)
  
  radio_setup();     
  set_timer(10000);
  uart_setup();
  
  

  while (1){
    if (rf_irq & RF24_IRQ_FLAGGED) {
      rf_irq &= ~RF24_IRQ_FLAGGED;
      msprf24_get_irq_reason();
    }
    if (rf_irq & RF24_IRQ_RX || msprf24_rx_pending()) {
      r_rx_payload(32, global_buf);
      msprf24_irq_clear(RF24_IRQ_RX);
      P2OUT ^= (1 << 6);
      
//      for(int i = 0; i < 6; i++) 
//        uart_putc(global_buf[i]);

      user = global_buf[0];
    } 
    else {
        user = 0xFF;
    }
  }
}

void radio_setup(void){
        char addr[5];
  	// Initial values for nRF24L01+ library config variables //
	rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
	rf_addr_width      = 5;
	rf_speed_power     = RF24_SPEED_1MBPS | RF24_POWER_0DBM;
	rf_channel         = 120;

	msprf24_init();
	msprf24_set_pipe_packetsize(0, 32);
	msprf24_open_pipe(0, 1);  // Open pipe#0 with Enhanced ShockBurst

	// Set our RX address
	addr[0] = 0xDE;	addr[1] = 0xAD;	addr[2] = 0xBE;	addr[3] = 0xEF;	addr[4] = 0x00;
	w_rx_addr(0, addr);

	// Receive mode
	if (!(RF24_QUEUE_RXEMPTY & msprf24_queue_state())) {
		flush_rx();
	}
	msprf24_activate_rx();
}

void set_timer(unsigned int delay_time){
  TA0CCR0 = delay_time;                    // Delay to allow Ref to settle
  TA0CCTL0 |= CCIE;                        // Compare-mode interrupt
  TA0CTL = TASSEL_2 + MC_1;                // TACLK = SMCLK, Up mode
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR(void){
  TA0CTL &= ~(MC_1); //stop
  
  uint8_t buf[8];
  for(int i = 0; i < 8 ; i++)
    buf[i] = 30;//global_buf[i];

  count = count + 1; // increment global counter
  if (count >= 31)   // if counter exeeds 31, claer
  {
    count = 0;
  }
                    // when the count is 0, turn all outputs on
  if(count == 0)
  {
    P6OUT |= (1<<0) | (1<<2) | (1<<3) | (1<<6) | (1<<7);
    P2OUT |= (1<<1) | (1<<2) | (1<<3);
  }
  
  
  //M1
  if(count == buf[6])
    P6OUT &= ~(1<<0);
  //M2
  if(count == buf[0])
    P6OUT &= ~(1<<3);
  //M3
  if(count == buf[1])
    P6OUT &= ~(1<<2);
  //M4
  if(count == buf[2])
    P6OUT &= ~(1<<7);  
  //M5
  if(count == buf[7])
    P6OUT &= ~(1<<6);
  //M6
  if(count == buf[3])
    P2OUT &= ~(1<<1);
  //M7
  if(count == buf[4])
    P2OUT &= ~(1<<3);
  //M8
  if(count == buf[5])
    P2OUT &= ~(1<<2);
    
  TA0CTL |= MC_1; //start it up again
}