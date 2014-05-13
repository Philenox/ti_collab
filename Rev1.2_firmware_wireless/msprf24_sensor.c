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
#include "nRF24L01.h"
#include "nrf_userconfig_sensor.h"
#include <stdint.h>

char rf_feature;  // Used to track which features have been enabled

/* CE (Chip Enable/RF transceiver activate signal) and CSN (SPI chip-select) operations. */

#define CSN_EN nrfCSNportout &= ~nrfCSNpin
#define CSN_DIS nrfCSNportout |= nrfCSNpin
#define CE_EN nrfCEportout |= nrfCEpin
#define CE_DIS nrfCEportout &= ~nrfCEpin

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
	UCA1IE = UCRXIE;
        UCA1TXBUF = inb;
	do {
		//LPM0;
	} while (UCA1STAT & UCBUSY);
	return UCA1RXBUF;
}

int spi_transfer16(int inw)
{
	int retw;

	UCA1IE = UCRXIE;
	UCA1TXBUF = (inw >> 8) & 0xFF;  // Send MSB first...
	do {
		//LPM0;
	} while (UCA1STAT & UCBUSY);
	
	retw = UCA1RXBUF << 8;
 
	UCA1IE = UCRXIE;
	UCA1TXBUF = inw & 0xFF;
	do {
		//LPM0;
	} while (UCA1STAT & UCBUSY);
        
	retw |= UCA1RXBUF;
	return retw;
}


/* Basic I/O to the device. */
unsigned char r_reg(unsigned char addr)
{
	int i;

	CSN_EN;
	i = spi_transfer16(RF24_NOP | ((addr & RF24_REGISTER_MASK) << 8));
	rf_status = (unsigned char) ((i & 0xFF00) >> 8);
	CSN_DIS;
	return (unsigned char) (i & 0x00FF);
}

void w_reg(unsigned char addr, char data)
{
	int i;
	CSN_EN;
	i = spi_transfer16( (data & 0x00FF) | (((addr & RF24_REGISTER_MASK) | RF24_W_REGISTER) << 8) );
	rf_status = (unsigned char) ((i & 0xFF00) >> 8);
	CSN_DIS;
}

void w_tx_addr(char *addr)
{
	int i;

	CSN_EN;
	rf_status = spi_transfer(RF24_TX_ADDR | RF24_W_REGISTER);
	for (i=rf_addr_width-1; i>=0; i--) {
		spi_transfer(addr[i]);
	}
	CSN_DIS;
}

void w_rx_addr(unsigned char pipe, char *addr)
{
	int i;

	if (pipe > 5)
		return;  // Only 6 pipes available
	CSN_EN;
	rf_status = spi_transfer((RF24_RX_ADDR_P0 + pipe) | RF24_W_REGISTER);
	if (pipe > 1) {  // Pipes 2-5 differ from pipe1's addr only in the LSB.
		spi_transfer(addr[rf_addr_width-1]);
	} else {
		for (i=rf_addr_width-1; i>=0; i--) {
			spi_transfer(addr[i]);
		}
	}
	CSN_DIS;
}

void w_tx_payload(unsigned char len, char *data)
{
	int i=0;
	CSN_EN;
	if (len % 2) {  // Odd payload size?  Make it even by stuffing the command in a 16-bit xfer
		// Borrowing 'i' to extract STATUS...
		i = spi_transfer16( (RF24_W_TX_PAYLOAD << 8) | (0x00FF & data[0]) );
		rf_status = (i & 0xFF00) >> 8;
		i = 1;
	} else {
		rf_status = spi_transfer(RF24_W_TX_PAYLOAD);
	}
	for (; i < len; i+=2) {
		// SPI transfers MSB first
		spi_transfer16( (data[i] << 8) | (0x00FF & data[i+1]) );
	}
	CSN_DIS;
}

void w_tx_payload_noack(unsigned char len, char *data)
{
	int i=0;

	if ( !(rf_feature & RF24_EN_DYN_ACK) )  // DYN ACK must be enabled to allow NOACK packets
		return;
	CSN_EN;
	if (len % 2) {
		// Borrowing 'i' to extract STATUS...
		i = spi_transfer16( (RF24_W_TX_PAYLOAD_NOACK << 8) | (0x00FF & data[0]) );
		rf_status = (i & 0xFF00) >> 8;
		i = 1;
	} else {
		rf_status = spi_transfer(RF24_W_TX_PAYLOAD_NOACK);
	}
	for (; i < len; i+=2) {
		// SPI transfers MSB first
		spi_transfer16( (data[i] << 8) | (0x00FF & data[i+1]) );
	}
	CSN_DIS;
}

unsigned char r_rx_peek_payload_size()
{
	int i;

	CSN_EN;
	i = spi_transfer16(RF24_NOP | (RF24_R_RX_PL_WID << 8));
	rf_status = (unsigned char) ((i & 0xFF00) >> 8);
	CSN_DIS;
	return (unsigned char) (i & 0x00FF);
}

unsigned char r_rx_payload(unsigned char len, char *data)
{
	int i=0,j;
	CSN_EN;
	if (len % 2) {
		// Borrowing 'i' to extract STATUS...
		i = spi_transfer16((RF24_R_RX_PAYLOAD << 8) | RF24_NOP);
		rf_status = (i & 0xFF00) >> 8;
		data[0] = i & 0x00FF;
		i = 1;
	} else {
		rf_status = spi_transfer(RF24_R_RX_PAYLOAD);
	}
	for (; i < len; i+=2) {
		j = spi_transfer16(0xFFFF);
		// SPI transfers MSB first
		data[i] = (j & 0xFF00) >> 8;
		data[i+1] = (j & 0x00FF);
	}
	CSN_DIS;
	// The RX pipe this data belongs to is stored in STATUS
	return ((rf_status & 0x0E) >> 1);
}

void flush_tx()
{
	CSN_EN;
	rf_status = spi_transfer(RF24_FLUSH_TX);
	CSN_DIS;
}

void flush_rx()
{
	CSN_EN;
	rf_status = spi_transfer(RF24_FLUSH_RX);
	CSN_DIS;
}

void tx_reuse_lastpayload()
{
	CSN_EN;
	rf_status = spi_transfer(RF24_REUSE_TX_PL);
	CSN_DIS;
}

inline void pulse_ce()
{
	CE_EN;
	__delay_cycles(DELAY_CYCLES_15US);
	CE_DIS;
}


void w_ack_payload(unsigned char pipe, unsigned char len, char *data)
{
	int i=0;
	CSN_EN;

	if (pipe > 5)
		return;
	if ( !(rf_feature & RF24_EN_ACK_PAY) )  // ACK payloads must be enabled...
		return;

	if (len % 2) {
		// Borrowing 'i' to extract STATUS...
		i = spi_transfer16( ((RF24_W_ACK_PAYLOAD | pipe) << 8) | (0x00FF & data[0]) );
		rf_status = (i & 0xFF00) >> 8;
		i = 1;
	} else {
		rf_status = spi_transfer(RF24_W_ACK_PAYLOAD | pipe);
	}
	for (; i < len; i+=2) {
		// SPI transfers MSB first
		spi_transfer16( (data[i] << 8) | (0x00FF & data[i+1]) );
	}
	CSN_DIS;
}


/* Configuration parameters used to set-up the RF configuration */
unsigned char rf_crc;
unsigned char rf_addr_width;
unsigned char rf_speed_power;
unsigned char rf_channel;
/* Status variable updated every time SPI I/O is performed */
unsigned char rf_status;
/* IRQ state is stored in here after msprf24_get_irq_reason(), RF24_IRQ_FLAGGED raised during
 * the IRQ port ISR--user application issuing LPMx sleep or polling should watch for this to
 * determine if the wakeup reason was due to nRF24 IRQ.
 */
volatile unsigned char rf_irq;





/* Library functions */
void msprf24_init()
{
	// Setup SPI
	spi_init();
	__bis_SR_register(GIE);  // Enable interrupts (set GIE in SR)

	// Setup IRQ
        P2DIR &= ~nrfIRQpin;  // IRQ line is input
        P2OUT |= nrfIRQpin;   // Pull-up resistor enabled
        P2REN |= nrfIRQpin;
        P2IES |= nrfIRQpin;   // Trigger on falling-edge
        P2IFG &= ~nrfIRQpin;  // Clear any outstanding IRQ
        P2IE |= nrfIRQpin;    // Enable IRQ interrupt

	// Setup CSN/CE ports
        P4DIR |= nrfCSNpin;
	CSN_DIS;

	P2DIR |= nrfCEpin;
	CE_DIS;

	/* Straw-man spi_transfer with no Chip Select lines enabled; this is to workaround errata bug USI5
	 * on the MSP430G2452 and related (see http://www.ti.com/lit/er/slaz072/slaz072.pdf)
	 * Shouldn't hurt anything since we expect no CS lines enabled by the user during this function's execution.
	 */
	spi_transfer(RF24_NOP);

	// Wait 100ms for RF transceiver to initialize.
	unsigned char c = 20;
	for (; c; c--) {
		__delay_cycles(DELAY_CYCLES_5MS);
	}

	// Configure RF transceiver with current value of rf_* configuration variables
	msprf24_irq_clear(RF24_IRQ_MASK);  // Forget any outstanding IRQs
	msprf24_close_pipe_all();          /* Start off with no pipes enabled, let the user open as needed.  This also
					    * clears the DYNPD register.
					    */
	msprf24_set_retransmit_delay(2000);  // A default I chose
	msprf24_set_retransmit_count(15);    // A default I chose
	msprf24_set_speed_power();
	msprf24_set_channel();
	msprf24_set_address_width();
	rf_feature = 0x00;  // Initialize this so we're starting from a clean slate
	msprf24_enable_feature(RF24_EN_DPL);      // Dynamic payload size capability (set with msprf24_set_pipe_packetsize(x, 0))
	msprf24_enable_feature(RF24_EN_DYN_ACK);  // Ability to use w_tx_payload_noack()

	msprf24_powerdown();
	flush_tx();
	flush_rx();
}

void msprf24_enable_feature(unsigned char feature)
{
	if ( (rf_feature & feature) != feature ) {
		rf_feature |= feature;
		rf_feature &= 0x07;  // Only bits 0, 1, 2 allowed to be set
		w_reg(RF24_FEATURE, rf_feature);
	}
}

void msprf24_disable_feature(unsigned char feature)
{
	if ( (rf_feature & feature) == feature ) {
		rf_feature &= ~feature;
		w_reg(RF24_FEATURE, rf_feature);
	}
}

void msprf24_close_pipe(unsigned char pipeid)
{
	unsigned char rxen, enaa;

	if (pipeid > 5)
		return;

	rxen = r_reg(RF24_EN_RXADDR);
	enaa = r_reg(RF24_EN_AA);

	rxen &= ~(1 << pipeid);
	enaa &= ~(1 << pipeid);

	w_reg(RF24_EN_RXADDR, rxen);
	w_reg(RF24_EN_AA, enaa);
}

void msprf24_close_pipe_all()
{
	w_reg(RF24_EN_RXADDR, 0x00);
	w_reg(RF24_EN_AA, 0x00);
	w_reg(RF24_DYNPD, 0x00);
}

void msprf24_open_pipe(unsigned char pipeid, unsigned char autoack)
{
	unsigned char rxen, enaa;

	if (pipeid > 5)
		return;

	rxen = r_reg(RF24_EN_RXADDR);
	enaa = r_reg(RF24_EN_AA);

	if (autoack)
		enaa |= (1 << pipeid);
	else
		enaa &= ~(1 << pipeid);
	rxen |= (1 << pipeid);
	w_reg(RF24_EN_RXADDR, rxen);
	w_reg(RF24_EN_AA, enaa);
}

unsigned char msprf24_pipe_isopen(unsigned char pipeid)
{
	unsigned char rxen;

	if (pipeid > 5)
		return 0;

	rxen = r_reg(RF24_EN_RXADDR);

	return ( (1<<pipeid) == (rxen & (1<<pipeid)) );
}

void msprf24_set_pipe_packetsize(unsigned char pipe, unsigned char size)
{
	unsigned char dynpdcfg;

	if (pipe > 5)
		return;

	dynpdcfg = r_reg(RF24_DYNPD);
	if (size < 1) {
		if ( !(rf_feature & RF24_EN_DPL) )  // Cannot set dynamic payload if EN_DPL is disabled.
			return;
		if (!( (1<<pipe) & dynpdcfg )) {
			// DYNPD not enabled for this pipe, enable it
			dynpdcfg |= 1 << pipe;
		}
	} else {
		dynpdcfg &= ~(1 << pipe);  // Ensure DynPD is disabled for this pipe
		if (size > 32)
			size = 32;
		w_reg(RF24_RX_PW_P0 + pipe, size);
	}
	w_reg(RF24_DYNPD, dynpdcfg);
}

void msprf24_set_retransmit_delay(int us)
{
	unsigned char c;

	// using 'c' to evaluate current RF speed
	c = rf_speed_power & RF24_SPEED_MASK;
	if (us > 4000)
		us = 4000;
	if (us < 1500 && c == RF24_SPEED_250KBPS)
		us = 1500;
	if (us < 500)
		us = 500;

	// using 'c' to save current value of ARC (auto-retrans-count) since we're not changing that here
	c = r_reg(RF24_SETUP_RETR) & 0x0F;
	us = (us-250) / 250;
	us <<= 4;
	w_reg(RF24_SETUP_RETR, c | (us & 0xF0));
}

void msprf24_set_retransmit_count(unsigned char count)
{
	unsigned char c;

	c = r_reg(RF24_SETUP_RETR) & 0xF0;
	w_reg(RF24_SETUP_RETR, c | (count & 0x0F));
}

unsigned char msprf24_get_last_retransmits()
{
	return r_reg(RF24_OBSERVE_TX) & 0x0F;
}

unsigned char msprf24_get_lostpackets()
{
	return (r_reg(RF24_OBSERVE_TX) >> 4) & 0x0F;
}

unsigned char _msprf24_crc_mask()
{
	return (rf_crc & 0x0C);
}

unsigned char _msprf24_irq_mask()
{
	return ~(RF24_MASK_RX_DR | RF24_MASK_TX_DS | RF24_MASK_MAX_RT);
}

unsigned char msprf24_is_alive()
{
	unsigned char aw;

	aw = r_reg(RF24_SETUP_AW);
	return((aw & 0xFC) == 0x00 && (aw & 0x03) != 0x00);
}

unsigned char msprf24_set_config(unsigned char cfgval)
{
	unsigned char previous_config;

	previous_config = r_reg(RF24_CONFIG);
	w_reg(RF24_CONFIG, (_msprf24_crc_mask() | cfgval) & _msprf24_irq_mask());
	return previous_config;
}

void msprf24_set_speed_power()
{
	if ( (rf_speed_power & RF24_SPEED_MASK) == RF24_SPEED_MASK )  // Speed setting RF_DR_LOW=1, RF_DR_HIGH=1 is reserved, clamp it to minimum
		rf_speed_power = (rf_speed_power & ~RF24_SPEED_MASK) | RF24_SPEED_MIN;
	w_reg(RF24_RF_SETUP, (rf_speed_power & 0x2E));
}

void msprf24_set_channel()
{
	if (rf_channel > 125)
		rf_channel = 0;
	w_reg(RF24_RF_CH, (rf_channel & 0x7F));
}

void msprf24_set_address_width()
{
	if (rf_addr_width < 3 || rf_addr_width > 5)
		return;
	w_reg(RF24_SETUP_AW, ((rf_addr_width-2) & 0x03));
}

unsigned char msprf24_current_state()
{
	unsigned char config;

	if (!msprf24_is_alive())               // Can't read/detect a valid value from SETUP_AW? (typically SPI or device fault)
		return RF24_STATE_NOTPRESENT;
	config = r_reg(RF24_CONFIG);
	if ( (config & RF24_PWR_UP) == 0x00 )  // PWR_UP=0?
		return RF24_STATE_POWERDOWN;
	if ( !(nrfCEportout & nrfCEpin) )      // PWR_UP=1 && CE=0?
		return RF24_STATE_STANDBY_I;
	if ( !(config & RF24_PRIM_RX) ) {      // PWR_UP=1 && CE=1 && PRIM_RX=0?
		if ( (r_reg(RF24_FIFO_STATUS) & RF24_TX_EMPTY) )  // TX FIFO empty?
			return RF24_STATE_STANDBY_II;
		return RF24_STATE_PTX; // If TX FIFO is not empty, we are in PTX (active transmit) mode.
	}
	if ( r_reg(RF24_RF_SETUP) & 0x90 )     // Testing CONT_WAVE or PLL_LOCK?
		return RF24_STATE_TEST;
	return RF24_STATE_PRX;                 // PWR_UP=1, PRIM_RX=1, CE=1 -- Must be PRX
}

// Power down device, 0.9uA power draw
void msprf24_powerdown()
{
	CE_DIS;
	msprf24_set_config(0);  // PWR_UP=0
}

// Enable Standby-I, 26uA power draw
void msprf24_standby()
{
	unsigned char state = msprf24_current_state();
	if (state == RF24_STATE_NOTPRESENT || state == RF24_STATE_STANDBY_I)
		return;
	CE_DIS;
	msprf24_set_config(RF24_PWR_UP);  // PWR_UP=1, PRIM_RX=0
	if (state == RF24_STATE_POWERDOWN) {  // If we're powering up from deep powerdown...
		//CE_EN;  // This is a workaround for SI24R1 chips, though it seems to screw things up so disabled for now til I can obtain an SI24R1 for testing.
		__delay_cycles(DELAY_CYCLES_5MS);  // Then wait 5ms for the crystal oscillator to spin up.
		//CE_DIS;
	}
}

// Enable PRX mode
void msprf24_activate_rx()
{
	msprf24_standby();
	// Purge any existing RX FIFO or RX interrupts
	flush_rx();
	w_reg(RF24_STATUS, RF24_RX_DR);

	// Enable PRIM_RX
	msprf24_set_config(RF24_PWR_UP | RF24_PRIM_RX);
	CE_EN;
	// 130uS required for PLL lock to stabilize, app can go do other things and wait
	// for incoming I/O.
}

// Enable Standby-II / PTX mode
/* Standby-II is enabled if the TX FIFO is empty, otherwise the chip enters PTX
 *     mode to send the TX FIFO buffer contents until it's all done, at which point
 *     the chip falls back to Standby-II again.
 */
void msprf24_activate_tx()
{
	msprf24_standby();
	// Cancel any outstanding TX interrupt
	w_reg(RF24_STATUS, RF24_TX_DS|RF24_MAX_RT);

	// Pulse CE for 10us to activate PTX
	pulse_ce();
}

/* Evaluate state of TX, RX FIFOs
 * Compare this with RF24_QUEUE_* #define's from msprf24.h
 */
unsigned char msprf24_queue_state()
{
	return r_reg(RF24_FIFO_STATUS);
}

/* Scan current channel for activity, produce an 8-bit integer indicating % of time
 * spent with RPD=1 (valid RF activity present) for a 133ms period.
 */
unsigned char msprf24_scan()
{
	int testcount = 1023;
	unsigned int rpdcount = 0;
	unsigned char last_state;

	last_state = msprf24_current_state();
	if (last_state != RF24_STATE_PRX)
		msprf24_activate_rx();
	for (; testcount > 0; testcount--) {
		if (r_reg(RF24_RPD))
			rpdcount++;
		__delay_cycles(DELAY_CYCLES_130US);
		flush_rx();
		w_reg(RF24_STATUS, RF24_RX_DR);  /* Flush any RX FIFO contents or RX IRQs that
						  * may have generated as a result of having PRX active.
						  */
	}
	if (last_state != RF24_STATE_PRX)
		msprf24_standby();  // If we weren't in RX mode before, leave it in Standby-I.
	return( (unsigned char) (rpdcount/4) );
}

// Check if there is pending RX fifo data
unsigned char msprf24_rx_pending()
{
	CSN_EN;
	rf_status = spi_transfer(RF24_NOP);
	CSN_DIS;

	if ((rf_status & 0x0E) < 0x0E)
		return 1;
	return 0;
}

// Get IRQ flag status
unsigned char msprf24_get_irq_reason()
{
	//rf_irq &= ~RF24_IRQ_FLAGGED;  -- Removing in lieu of having this check determined at irq_clear() time
	CSN_EN;
	rf_status = spi_transfer(RF24_NOP);
	CSN_DIS;
	rf_irq = rf_status & RF24_IRQ_MASK;
	return rf_irq;
}

/* Clear IRQ flags */
void msprf24_irq_clear(unsigned char irqflag)
{
	rf_irq &= ~RF24_IRQ_FLAGGED;  /* We clear this, then decide later if it should be re-set.
				       * Helps avoid race conditions where an IRQ fires between STATUS read
				       * and handling of the RF24_IRQ_FLAGGED bit.
				       */
        CSN_EN;
	rf_status = spi_transfer(RF24_STATUS | RF24_W_REGISTER);
	if (irqflag & RF24_IRQ_RX && ((rf_status & 0x0E) < 0x0E))
		irqflag &= ~RF24_IRQ_RX;  // If there is still RX data pending, prevent user from clearing RX IRQ.
	spi_transfer(irqflag);
        CSN_DIS;

	rf_irq = (rf_irq & ~RF24_IRQ_MASK) | (rf_status & RF24_IRQ_MASK & ~irqflag);
	if (rf_irq & RF24_IRQ_MASK)
		rf_irq |= RF24_IRQ_FLAGGED;
}

/*      -       -       Interrupt vectors       -       -       */

#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_RX(void) {
	UCA1IE &= ~UCRXIE;
	//__bic_SR_register_on_exit(LPM0_bits);  // Clear LPM0 mode
}


// RF transceiver IRQ handling
#pragma vector = PORT2_VECTOR
__interrupt void P1_IRQ (void) {
        if(P2IFG & nrfIRQpin){
                //__bic_SR_register_on_exit(LPM4_bits);    // Wake up
		rf_irq |= RF24_IRQ_FLAGGED;
		P2IFG &= ~nrfIRQpin;   // Clear interrupt flag
        }
}


