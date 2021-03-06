/*
* Slave 1.c
*
* Created: 7-2-2018 11:38:04
* Author : Max
*/

#define F_CPU 32000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sfr_defs.h>
#include <stddef.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include "../20180702 - 2 Slave 1 Master/nrf24spiXM2.h"
#include "../20180702 - 2 Slave 1 Master/nrf24L01.h"
#include "../20180702 - 2 Slave 1 Master/clock.h"
#include "../20180702 - 2 Slave 1 Master/stream.h"
#include <ctype.h>

#define DEBOUNCE_PERIOD_MS	10		//
#define LOCK_PERIOD_MS		200		// bepaalt voor hoelang je de knop niet kan indrukken

#define SENSOR_NAME			"S1"	// bepaalt de naam van de slave

// twee pipes voor read & transmit
uint8_t  pipes[][6] = {
	"AVH01",
	"AVH02"
};

uint8_t  packet[32];

void init_pwm(void);
void init_nrf(void);
void init_adc(void);
void read_adcs(uint16_t *res);
int button_pressed(void);
void append(char* s, char c);

int main(void)
{
	Config32MHzClock_Ext16M();
	init_adc();
	init_pwm();
	init_nrf();

	init_stream(F_CPU);
	PMIC.CTRL |= PMIC_LOLVLEN_bm;           // Low level interrupt
	sei();

	clear_screen();		// leegt de scherm op de pc
	
	// overal van er printf staat komt er een zin te schrijnen op de PC. Dit gebruik ik om de xMega te testen
	printf("Sensor %s (Slave)\n", SENSOR_NAME);

	// oneindig loop (1 = true, dus while(true))
	while (1) {
		if ( button_pressed() ) {
			printf("Button pressed!\n");
			
			char command[256] = SENSOR_NAME;
			// P staat voor Pressed, dat is een stuk van een protocol die ik heb bedacht
			char c = 'P';

			// append is toevoegen, dus voeg 'c' aan 'command'. Je krijgt dan S1P
			append(command, c);

			// stuur de command (S1P) over de radio
			nrfWrite( (uint8_t *) command, strlen(command) );
			
			_delay_ms(LOCK_PERIOD_MS);       // lock input
		}
	}
}

ISR(PORTF_INT0_vect)
{
	uint8_t tx, fail, rx;
	static uint8_t  message_count = 0;

	nrfWhatHappened(&tx, &fail, &rx);
	
	printf("interrupt : tx(%d) fail(%d) rx(%d)\n", tx, fail, rx);
}

void init_pwm(void)
{
	PORTD.DIRCLR   = PIN3_bm;            // input pin switch
	PORTD.PIN3CTRL = PORT_OPC_PULLUP_gc; // enable pull up
}

void init_nrf(void)
{
	nrfspiInit();
	
	nrfBegin();

	nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_8RETRANSMIT_gc);
	nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);
	nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);
	nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
	nrfSetChannel(48);
	nrfSetAutoAck(1);
	nrfEnableDynamicPayloads();

	nrfClearInterruptBits();
	nrfFlushRx();
	nrfFlushTx();

	// Interrupt Pin
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL  = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL  |= // (PORTF.INTCTRL & ~PORT_INT0LVL_gm) |
	PORT_INT0LVL_LO_gc ;  // Interrupts Low Level

	// Opening pipes
	nrfOpenWritingPipe(pipes[1]);
	nrfOpenReadingPipe(1,pipes[0]);
	nrfStartListening();
}

int button_pressed(void)
{
	if ( bit_is_clear(PORTD.IN,PIN3_bp) ) {
		_delay_ms(DEBOUNCE_PERIOD_MS);
		while ( bit_is_clear(PORTD.IN,PIN3_bp) ) ;
		return 1;
	}
	return 0;
}

void append(char* s, char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}
