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
#include "nrf_userconfig_feedback.h"
#include "stdint.h"
#include "UART.h"

void initialize_pwm (void);
void set_timer(unsigned int delay_time);

volatile unsigned int user;
char buf[32];
uint8_t increment;

void main()
{
	char addr[5];

	WDTCTL = WDTHOLD | WDTPW;
             
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
        
        initialize_pwm();
        set_timer(200);
  
	LPM4;
	while (1) {
		if (rf_irq & RF24_IRQ_FLAGGED) {
			rf_irq &= ~RF24_IRQ_FLAGGED;
			msprf24_get_irq_reason();
		}
		if (rf_irq & RF24_IRQ_RX || msprf24_rx_pending()) {
			r_rx_payload(32, buf);
			msprf24_irq_clear(RF24_IRQ_RX);
			user = buf[0];
		} 
                else {
			user = 0xFF;
		}
		LPM4;
	}
}

