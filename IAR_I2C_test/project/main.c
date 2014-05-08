
#include "io430.h"
//#include <msp430f5528.h>
#include "uart.h"

#define RX_BUFF_SIZE 6
#define SLAVEADDRESS 0x1E;

unsigned char tx_cnt;
unsigned char *p_tx_data;

unsigned char *p_rx_data;	
volatile unsigned char rx_buff[RX_BUFF_SIZE];
 
char rx_front = 0;
char rx_back = 0;

int clock_init(void){
  UCSCTL0 |= (31 <<8); // set DCO to 31
  UCSCTL1 |= DCORSEL0 + DCORSEL1; // frequency range
  UCSCTL3 |= SELREF1; //set fll reference base on REFOCLK
  UCSCTL2 |= FLLD0 + FLLD1 + 31; //set FLLD = 8, FLLN = 31, 32.756KHz * 8 *31 = 8MHz
  UCSCTL4 = SELA0 + SELA1 + SELS0 + SELS1 + SELM0 + SELM1; //select the DCO clock as the source for SCLK, MCLK and ACLK
  
  return 1;  
}

void i2c_init (void){
  P4SEL |= (1 << 1) | (1 << 2);
  
  UCB1CTL1 = UCSWRST;      // Enable SW reset   
  
  // UCBxCTL0 
  // BIT
  //  7         UCA10 - 0 7-bit own address, 1 10-bit own address
  //  6         UCSLA10 - 0 7-bit slave address, 1 10-bit slave address
  //  5         UCMM - 0 Single master, 1 Multi-master
  //  4         Reserved
  //  3         UCMST - 0 Slave mode, 1 Master mode
  //  2-1       UCMODE_3 - 11 I2C mode
  //  0         USCYNC - 0 asynch mode, 1 synch mode

  // UCBxCTL1
  // BIT
  // 7-6        UCSSELx - 00, UCLKI, 01 ACLK, 10 and 11 SMCLK
  // 5          Reserved
  // 4 972      UCTR - 0 Receiver, 1 Transmitter
  // 3          UCTXNACK - 0 Acknowledge normally, 1 generate NACK
  // 2          UCTXSTP - 0 No STOP is generated, 1 generate STOP
  // 1          UCTXSTT - 0 Do not generate START condition, 1 generate START condition
  // 0          UCSWRST - 0 normal operation, 1 held in reset state
  
  UCB1CTL0 = UCMST | UCMODE_3 | UCSYNC; //Master mode, I2C mode, Synchronous 
  UCB1CTL1 = UCSSEL__SMCLK | (1 << 4); //Clock source SMCLK, Transmitter
  
  // I2C Prescaler 8MHz/20 = 400KHz clock
  UCB1BR0 = 0x14;
  UCB1BR1 = 0x00;
  UCB1I2CSA = SLAVEADDRESS;   
  UCB1CTL1 &= ~UCSWRST;                       // Clear SW reset, resume operation

  UCB1IE |= UCRXIE + UCTXIE;	//recieve, transmit interrupt enable
}

void i2c_start(void){
  //Generate START condition
  UCB1CTL1 |= UCTXSTT; 
//  UCB1I2CSA = SLAVEADDRESS;
}

void i2c_stop(void){
  //Generate STOP condition
  UCB1CTL1 |= UCTXSTP;
}

void i2c_send (unsigned char *data, unsigned char num_byte){
  UCB1CTL1 |= UCTR + UCTXSTT;	//i2c TX, start condition
  
  while(num_byte){
    UCB1TXBUF = data[num_byte];
//  while(UCB1CTL1 & UCTXSTP);	//ensure stop condition was sent  
    num_byte--;
  }
}

void i2c_put(char *tx_buf, char num_byte){	//pointer to TX data
	
	p_tx_data = (unsigned char *) tx_buf;	//pass pointer
	tx_cnt = num_byte;	//number of bytes to transmit
	
	UCB1CTL1 |= UCTR + UCTXSTT;	//i2c TX, start condition
	while(UCB1CTL1 & UCTXSTP);	//ensure stop condition was sent
}

//call in while loop until 0 is received
char i2c_get(void){
        //delay 10 us
	__delay_cycles(800);
	char temp;
	if(rx_back != rx_front){
		p_rx_data = (unsigned char*) rx_buff;
		temp = rx_buff[rx_back];
		rx_back++;
		if(rx_back > RX_BUFF_SIZE){
			rx_back = 0;
		}
		return temp;
	}
	return 0;
}

#pragma vector = USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void){
  switch(__even_in_range(UCB1IV, 12)){
    case 0: break;
    //no pending interrupt
    case 2: break;
    //Arbitration lost; UCALIFG
    case 4: break;
    //NACK
    case 6: break;
    //Start condition received
    case 8: break;
    //stop condition received
    case 10:	//data received 
      *p_rx_data++ = UCB0RXBUF;		//get RX'd byte into buffer
      rx_front++;
      if(rx_front > RX_BUFF_SIZE){
        rx_front = 0;
      }
      break; 
		
    case 12:	//transmit buffer empty 
      if(tx_cnt){
        UCB1TXBUF = *p_tx_data;	//load TX buffer
        tx_cnt--;	        //dec TX byte counter
      } else {
        UCB1CTL1 |= UCTXSTP;	//i2c stop condition
        UCB1IFG &= ~UCTXIFG;	//clear USCI_B1 TX int flag
      }
                  
      break;
		
    default: break;	
  }
}


int main(void){
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  
  // Set up magnetometer to continuos measurement mode
  unsigned char continuousMode[2] = {0x02, 0x00}; 
  
  
  int i = 0;
  
  clock_init();
  uart_setup();
  i2c_init();
  
  P2DIR |= (1 << 6);

//  i2c_put(continuousMode, 2);
  
  while(1){
    P2OUT ^= (1 << 6);   

  i2c_send(continuousMode, 2);
//  i2c_stop();
  
/*    
    while(i2c_get());
 
    for(i = 0; i < 6; i++){
      uart_putc(rx_buff[i]);
      __delay_cycles(800);      
    }
*/    
    __delay_cycles(8000000);

  }

}
