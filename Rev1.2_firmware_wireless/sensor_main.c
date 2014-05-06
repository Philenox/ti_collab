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
#include "msprf24.h"
#include "nrf_userconfig_sensor.h"
#include "stdint.h"

volatile unsigned int user;
void radio_init();

void main()
{
	char buf[32];

        WDTCTL = WDTHOLD | WDTPW;
        
        UCSCTL0 |= (31 <<8); // set DCO to 31
        UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
        UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
        UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
        UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
              
        radio_init();
        
        P2DIR |= (1<<2); //LED debug pin
	while(1){
		__delay_cycles(100000);
		buf[0]= 255;
		w_tx_payload(32, buf);
		msprf24_activate_tx();
		LPM4;

		if (rf_irq & RF24_IRQ_FLAGGED) {
			rf_irq &= ~RF24_IRQ_FLAGGED;

			msprf24_get_irq_reason();
			if (rf_irq & RF24_IRQ_TX){
                          P2OUT |= (1<<2);
			}
			if (rf_irq & RF24_IRQ_TXFAILED){
                          P2OUT ^= (1<<2);
			}

			msprf24_irq_clear(rf_irq);
			user = msprf24_get_last_retransmits();
		}
	}
}

void radio_init(void)
{
    char addr[5];
    rf_crc = RF24_EN_CRC | RF24_CRCO; // CRC enabled, 16-bit
    rf_addr_width      = 5;
    rf_speed_power     = RF24_SPEED_1MBPS | RF24_POWER_0DBM;
    rf_channel         = 120;

    msprf24_init();  // All RX pipes closed by default
    msprf24_set_pipe_packetsize(0, 32);
    msprf24_open_pipe(0, 1);  // Open pipe#0 with Enhanced ShockBurst enabled for receiving Auto-ACKs

    // Transmit to 'rad01' (0x72 0x61 0x64 0x30 0x31)
    msprf24_standby();
    user = msprf24_current_state();
    addr[0] = 0xDE;	addr[1] = 0xAD;	addr[2] = 0xBE;	addr[3] = 0xEF;	addr[4] = 0x00;
    w_tx_addr(addr);
    w_rx_addr(0, addr);  // Pipe 0 receives auto-ack's, autoacks are sent back to the TX addr so the PTX node
                                 // needs to listen to the TX addr on pipe#0 to receive them.
}
