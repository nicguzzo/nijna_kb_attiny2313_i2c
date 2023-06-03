#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define USI_DATA        USIDR
#define USI_STATUS      USISR
#define USI_CONTROL     USICR
#define USI_ADDRESS     0x8

#define NONE          0
#define ACK_PR_RX     1
#define BYTE_RX       2
#define ACK_PR_TX     3
#define PR_ACK_TX     4
#define BYTE_TX       5

#define DDR_USI             DDRB
#define PORT_USI            PORTB
#define PIN_USI             PINB
#define PORT_USI_SDA        PORTB5
#define PORT_USI_SCL        PORTB7

volatile uint8_t COMM_STATUS = NONE;
#ifdef CORNE
#define COLS 6
#define ROWS 4
#define N_BYTES 3
#define N_KEYS 24 
volatile uint8_t cols[COLS]={_BV(PORTD0),_BV(PORTD1),_BV(PORTD4),_BV(PORTD2),_BV(PORTD3),_BV(PORTD5)};
volatile uint8_t rows[ROWS]={_BV(PORTB0),_BV(PORTB1),_BV(PORTB2),_BV(PORTB3)};
#endif
#ifdef NINJA2
#define COLS 6
#define ROWS 5
#define N_BYTES 4
#define N_KEYS 30
volatile uint8_t cols[COLS]={_BV(PORTD0),_BV(PORTD1),_BV(PORTD4),_BV(PORTD2),_BV(PORTD3),_BV(PORTD5)};
volatile uint8_t rows[ROWS]={_BV(PORTB0),_BV(PORTB1),_BV(PORTB2),_BV(PORTB3),_BV(PORTB4)};
#endif
volatile uint8_t bytes[N_BYTES]={0};
volatile uint8_t t=0;



void USI_init(void) {
  // 2-wire mode; Hold SCL on start and overflow; ext. clock
  USI_CONTROL |= (1<<USIWM1) | (1<<USICS1);
  USI_STATUS = 0xf0;  // write 1 to clear flags, clear counter
  DDR_USI  &= ~(1<<PORT_USI_SDA);
  PORT_USI &= ~(1<<PORT_USI_SDA);
  DDR_USI  |=  (1<<PORT_USI_SCL);
  PORT_USI |=  (1<<PORT_USI_SCL);
  // startcondition interrupt enable
  USI_CONTROL |= (1<<USISIE);
}

int main(void) {
  
  DDRD |= _BV(PORTD6); //test led
  PORTD &= ~_BV(PORTD6);
  
  //port D as outputs  
  DDRD |= _BV(PORTD0);
  DDRD |= _BV(PORTD1);
  DDRD |= _BV(PORTD2);
  DDRD |= _BV(PORTD3);
  DDRD |= _BV(PORTD4);
  DDRD |= _BV(PORTD5);
  PORTD |= _BV(PORTD0);
  PORTD |= _BV(PORTD1);
  PORTD |= _BV(PORTD2);
  PORTD |= _BV(PORTD3);
  PORTD |= _BV(PORTD4);
  PORTD |= _BV(PORTD5);

  //set port b as inputs
  DDRB &= ~_BV(PORTB0);
  DDRB &= ~_BV(PORTB1);
  DDRB &= ~_BV(PORTB2);
  DDRB &= ~_BV(PORTB3);
  #ifdef NINJA2
  DDRB &= ~_BV(PORTB4);
  #endif
  //enable internal pull-ups on port b pins
  PORTB |= _BV(PORTB0);
  PORTB |= _BV(PORTB1);
  PORTB |= _BV(PORTB2);
  PORTB |= _BV(PORTB3);
  #ifdef NINJA2
  PORTB |= _BV(PORTB4);
  #endif

  USI_init();
  sei();  

  for(;;) { 
    PORTD |= _BV(PORTD6);
    _delay_ms(100);
    PORTD &= ~_BV(PORTD6);
    _delay_ms(100);
  }

}

ISR(USI_START_vect) {
  //uint8_t tmpUSI_STATUS;
  //tmpUSI_STATUS = USI_STATUS;
  COMM_STATUS = NONE;
  // Wait for SCL to go low to ensure the "Start Condition" has completed.
  // otherwise the counter will count the transition
  while ( (PIN_USI & (1<<PORT_USI_SCL)) );
  USI_STATUS = 0xf0; // write 1 to clear flags; clear counter
  // enable USI interrupt on overflow; SCL goes low on overflow
  USI_CONTROL |= (1<<USIOIE) | (1<<USIWM0);
}


ISR(USI_OVERFLOW_vect) {
  uint8_t BUF_USI_DATA = USI_DATA;
  switch(COMM_STATUS) {
  case NONE:
    if (((BUF_USI_DATA & 0xfe) >> 1) != USI_ADDRESS) {  // if not receiving my address
      // disable USI interrupt on overflow; disable SCL low on overflow
      USI_CONTROL &= ~((1<<USIOIE) | (1<<USIWM0));
    }
    else { // else address is mine
      DDR_USI  |=  (1<<PORT_USI_SDA);
      USI_STATUS = 0x0e;  // reload counter for ACK, (SCL) high and back low
      if (BUF_USI_DATA & 0x01) COMM_STATUS = ACK_PR_TX; else COMM_STATUS = ACK_PR_RX;
    }
    break;
  case ACK_PR_RX:
    DDR_USI  &= ~(1<<PORT_USI_SDA);
    COMM_STATUS = BYTE_RX;
    break;
  case BYTE_RX:
    // Save received byte here! ... = USI_DATA
    DDR_USI  |=  (1<<PORT_USI_SDA);
    USI_STATUS = 0x0e;  // reload counter for ACK, (SCL) high and back low
    COMM_STATUS = ACK_PR_RX;
    break;
  case ACK_PR_TX:
    // Put first byte to transmit in buffer here! USI_DATA = ...


    uint16_t byte_index=0;
    uint16_t byte=0;
    uint16_t bit=0;
    uint16_t col=0;
    uint16_t row=0;
    for(col=0;col<COLS;col++){
      PORTD &= ~cols[col];
      for(row=0;row<ROWS;row++){
        byte_index=row*COLS+col;
        byte=byte_index>>3;// divide by 8
        bit=(byte_index%8);
        if( ((PINB)&rows[row])==0) {
            bytes[byte]|=1<<bit;
        }else{
            bytes[byte]&= ~(1<<bit);
        }
      }
      PORTD |= cols[col];
    }    

    PORTD &= ~_BV(PORTD6);

    t=0;
    USI_DATA =bytes[t];    
    PORT_USI |=  (1<<PORT_USI_SDA); // transparent for shifting data out
    COMM_STATUS = BYTE_TX;
    t++;
    break;
  case PR_ACK_TX:
    if(BUF_USI_DATA & 0x01) {
      COMM_STATUS = NONE; // no ACK from master --> no more bytes to send
    }
    else {
      // Put next byte to transmit in buffer here! USI_DATA = ...
      USI_DATA =bytes[t];      
      PORT_USI |=  (1<<PORT_USI_SDA); // transparent for shifting data out
      DDR_USI  |=  (1<<PORT_USI_SDA);
      COMM_STATUS = BYTE_TX;
      if(t<N_BYTES){        
        t++;
      }
    }
    break;
  case BYTE_TX:
    DDR_USI  &= ~(1<<PORT_USI_SDA);
    PORT_USI &= ~(1<<PORT_USI_SDA);
    USI_STATUS = 0x0e;  // reload counter for ACK, (SCL) high and back low
    COMM_STATUS = PR_ACK_TX;
    break;
  }
  USI_STATUS |= (1<<USIOIF); // clear overflowinterruptflag, this also releases SCL
}
