/*	Author: Rishab Dudhia (rdudh001)
 *  Partner(s) Name: 
 *	Lab Section: 022
 *	Assignment: Lab #8  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *	Three buttons to output three different tones
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *	Youtube link: https://www.youtube.com/watch?v=6qfnKBpKBuU
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

//0.954 hz is lowest frequency possible with this function
//based on settings in PWM_on()
//Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(double frequency) {
	static double current_frequency; //keeps track of the currently set frequency
	//Will only update the registers when the frequency changes, otherwise allows 
	//music to play uninterrupted
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } // stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter

		//prevents OCR3A from overflowing, using prescaler 64
		//0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0XFFFF; }

		//prevents OCR3A from underflowing, using prescaler 64
		//31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }

		//set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) -1; }

		TCNT3 = 0; //resets counter
		current_frequency = frequency; // updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
		// COM3A0: toggle PB3 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
		//WGM32: when counter (TCNT3) matches OCR3A, reset counter
		//CS31 & CS30: set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

volatile unsigned char TimerFlag = 0; // TimerISR() sets to 1

unsigned long _avr_timer_M = 1; //start count from here, down to 0. default 1 ms
unsigned long _avr_timer_cntcurr = 0; //current internal count of 1ms ticks

void TimerOn () {
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit3 = 0: CTC mode (clear timer on compare)
		       //bit2bit1bit0 = 011: pre-scaler / 64
		       //00001011: 0x0B
		       //so, 8 MHz clock or 8,000,000 / 64 = 125,000 ticks/s
		       //thus, TCNT1 register will count at 125,000 ticks/s
	//AVR output compare register OCR1A
	OCR1A = 125; //timer interrupt will be generated when TCNT1 == OCR1A
	             //we want a 1 ms tick. 0.001s * 125,000 ticks/s = 125
		     //so when TCNT1 register equals 125,
		     //1 ms has passed. thus, we compare 125.
		     
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR will be called every _avr_timer_cntcurr milliseconds
	
	//Enable global interrupts
	SREG |= 0x80; // 0x80: 10000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0 = 000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

//In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect){
	//CPU automatically calls when TCNT1 == OCR1 (every 1ms per TimerOn settings)
	_avr_timer_cntcurr--; // count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

//Set TimerISR() to tick every M ms
void TimerSet(unsigned long M){
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

enum States {smstart, off, playC, playD, playE} state;

void Tick() {
	unsigned char inA = ~PINA & 0x07;
	switch (state) {
		case smstart:
			state = off;
			break;
		case off:
			if (inA == 0x01)
				state = playC;
			else if (inA == 0x02)
				state = playD;
			else if (inA == 0x04)
				state = playE;
			else
				state = off;
			break;
		case playC:
			if (inA == 0x01)
				state = playC;
			else
				state = off;
			break;
		case playD:
			if (inA == 0x02)
				state = playD;
			else
				state = off;
			break;
		case playE:
			if (inA == 0x04)
				state = playE;
			else
				state = off;
			break;
		default:
			state = off;
			break;
	}

	switch (state) {
		case smstart:
			break;
		case off:
			set_PWM(0);
			break;
		case playC:
			set_PWM(261.63);
			break;
		case playD:
			set_PWM(293.66);
			break;
		case playE:
			set_PWM(329.63);
			break;
		default:
			set_PWM(0);
			break;
	}
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;
    PWM_on();
    TimerSet(200);
    TimerOn();
    state = smstart;
    /* Insert your solution below */
    while (1) {
	Tick();
	while(!TimerFlag);
	TimerFlag = 0;
    }
    return 1;
}
