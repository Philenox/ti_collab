/* nrf_userconfig.h
 * User configuration of nRF24L01+ connectivity parameters, e.g.
 * IRQ, CSN, CE pin assignments, Serial SPI driver type
 *
 *
 * Copyright (c) 2012, Eric Brundick <spirilis@linux.com>
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

#ifndef _NRF_USERCONFIG_H
#define _NRF_USERCONFIG_H

/* CPU clock cycles for the specified amounts of time--accurate minimum delays
 * required for reliable operation of the nRF24L01+'s state machine.
 */

// Settings for 8MHz MCLK.
#define DELAY_CYCLES_5MS       40000
#define DELAY_CYCLES_130US     1040
#define DELAY_CYCLES_15US      120


/* SPI port--Select which USCI port we're using.
 * Applies only to USCI devices.  USI users can keep these
 * commented out.
 */

/* Define whether this library should use LPM0+IRQs during SPI I/O and whether this library should provide the ISR. */
/* Operational pins -- IRQ, CE, CSN (SPI chip-select)
 */

/* IRQ */
#define nrfIRQport 1
#define nrfIRQpin BIT5

/* CSN SPI chip-select */
#define nrfCSNport 4
#define nrfCSNportout P4OUT
#define nrfCSNpin BIT3

/* CE Chip-Enable (used to put RF transceiver on-air for RX or TX) */
#define nrfCEport 1
#define nrfCEportout P1OUT
#define nrfCEpin BIT4

#endif
