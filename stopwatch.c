#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "main.h"
#include "lcd_drv.h"

// These are included by the LCD driver code, so 
// we don't need to include them here.
// #include <avr/io.h>
// #include <util/delay.h>

volatile uint8_t hours = 0;
volatile uint8_t minutes = 0;
volatile uint8_t seconds = 0;
volatile uint8_t sub_seconds = 0;

volatile uint16_t current_button_val = 1001; // default no button pressed value

#define TOP (uint16_t) (0.5+(F_CPU/(1024.0*100)))

// triggers every 0.01s when timer 1 reaches top value
ISR(TIMER1_COMPA_vect) {
	sub_seconds++; // increments every 0.01 seconds
	
	if (sub_seconds >= 100) {
		sub_seconds = 0;
		seconds++;
		
		if (seconds >= 60) {
			seconds = 0;
			minutes++;
			
			if (minutes >= 60) {
				minutes = 0;
				hours++;
			}
			
			if (hours >= 24) { // 24 hour format. or that i assumed this is what the writeup means by 24 hours format...though i thought a timer doesnt really reset the hour when it hits 24
				hours = 0;
			}
		}
	}
}

// triggers when the adc finishes reading button voltage
ISR(ADC_vect) {
	current_button_val = ADC;
}


void timer_init() {
	OCR1A = TOP;
	TCCR1A = 0;
	TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10); // set ctc mode and prescaler to 1024
	TIMSK1 |= (1<<OCIE1A);
}

int main( void )
{
	timer_init();
	lcd_init();
	
	// set up ADC converter:
	// ADEN - enable ADC
	// ADATE - auto trigger enable
	// ADIE - adc interrupt enable 
	// set prescaler to 128
	ADCSRA = (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); 
	ADMUX = 0x40; // ADC0
	
	ADCSRA |= 0x40; // start conversion
	sei();
	
	uint8_t line1 = 1; // keep running state for line 1
	uint8_t line2 = 1; // keep-running state for line 2
	uint8_t save_button = 0; // save the state of the last button clicked or if no button has been clicked
			
	char display_buffer[17]; // buffer for displaying
	
	for (;;)
	{	
		cli(); // disable and enable interrupts so timer isr wont change the variables halfway
		uint8_t h = hours;
		uint8_t m = minutes;
		uint8_t s = seconds;
		uint8_t ss = sub_seconds;
		sei();
		
		if (current_button_val > 1000) {
			save_button = 0; // no button pressed
		}else if (save_button == 0) {
			save_button = 1;
			if (current_button_val < 50) {
				// RIGHT
			}else if (current_button_val < 195) {
				line1 = 0; // UP
			}else if (current_button_val < 380) {
				line1 = 1; // DOWN
				line2 = 1;
			}else if (current_button_val < 555) {
				line2 = 0; // LEFT
			}
		}
		
		// display the active timer. if button pressed, pause
		if (line1 == 1) {
			sprintf(display_buffer, "%02d:%02d:%02d.%03d", h, m, s ,ss * 10);
			lcd_xy(0, 0);
			lcd_puts(display_buffer);
		}
		if (line2 == 1) {
			sprintf(display_buffer, "%02d:%02d:%02d.%03d", h, m, s ,ss * 10);
			lcd_xy(0, 1);
			lcd_puts(display_buffer);
		}
	}
	return 0;
}
