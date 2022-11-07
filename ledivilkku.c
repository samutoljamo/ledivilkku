#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdlib.h>

#include "led.h"

// Students notice: This program uses global variables, which would be bad design for any larger systems than this.
// Not that the design were particularly elegant to begin with

void setup(void);
void tick(void);
void matrix(void);
void animate(void);
void setanimation(void);
uint16_t animate_parsevalue(uint8_t digits);
uint8_t animate_hex2dec(uint8_t character);
void powerdown(void);

#define OVERFLOW_COUNT 122

const uint8_t number_matrix[10][15] PROGMEM = {
	{15, 15, 15,
	 15, 0, 15,
	 15, 0, 15,
	 15, 0, 15,
	 15, 15, 15},
	{0, 0, 15,
	 0, 0, 15,
	 0, 0, 15,
	 0, 0, 15,
	 0, 0, 15},
	{15, 15, 15,
	 0, 0, 15,
	 15, 15, 15,
	 15, 0, 0,
	 15, 15, 15},
	{15, 15, 15,
	 0, 0, 15,
	 15, 15, 15,
	 0, 0, 15,
	 15, 15, 15},
	{15, 0, 15,
	 15, 0, 15,
	 15, 15, 15,
	 0, 0, 15,
	 0, 0, 15},
	{15, 15, 15,
	 15, 0, 0,
	 15, 15, 15,
	 0, 0, 15,
	 15, 15, 15},
	{15, 15, 15,
	 15, 0, 0,
	 15, 15, 15,
	 15, 0, 15,
	 15, 15, 15},
	{15, 15, 15,
	 0, 0, 15,
	 0, 0, 15,
	 0, 0, 15,
	 0, 0, 15},
	{15, 15, 15,
	 15, 0, 15,
	 15, 15, 15,
	 15, 0, 15,
	 15, 15, 15},
	{15, 15, 15,
	 15, 0, 15,
	 15, 15, 15,
	 0, 0, 15,
	 0, 0, 15}};

const uint8_t number_map[15] PROGMEM = {0, 1, 2, 16, 17, 18, 32, 33, 34, 48, 49, 50, 64, 65, 66};


void set_number(uint8_t number, uint8_t offset_x, uint8_t offset_y)
{
	number = number % 10;
	for (uint8_t i = 0; i < 15; i++)
	{
		l[pgm_read_byte(&number_map[i]) + offset_x + offset_y * 16] = pgm_read_byte(&number_matrix[number][i]);
	}
}

void show_num(uint8_t number)
{
	number = number % 100; // prevent weird things
	uint8_t first_num = number % 10;
	uint8_t second_num = number / 10;
	set_number(first_num, 13, 6);
	set_number(second_num, 9, 6);
}

void show_mode(uint8_t m)
{
	m = m % 5;
	l[16 * 6 + 16 * m] = 15;
}

void setup_mode(uint8_t mode)
{
	if (mode == 0)
	{
		show_num(0);
	}
	else if (mode == 1)
	{
		show_num(99);
	}
	l2led();
	tick();
}
uint8_t mode = 0;
uint8_t mode_state = 0; // used to track state inside a mode
uint8_t mode_num = 0;

uint8_t overflow_count = 0;

void start_timer()
{
	TCCR0 = (1 << CS02);
	TCNT0 = 0;
	overflow_count = 0;
}

// 8-bit timer thats why the hack
ISR(TIMER0_OVF_vect)
{
	if (overflow_count == OVERFLOW_COUNT)
	{
		if (mode_state == 1 && mode_num < 99)
			mode_num++;
		overflow_count = 0;
	}
	else
	{
		overflow_count++;
	}
}

int main(void)
{
	uint8_t pushcount;
	void stop_timer()
	{
	}
	setup();
	for (uint8_t i = 0; i < 20; i++)
		tick();
	while (led_button)
		;
	for (uint8_t i = 0; i < 20; i++)
		tick();

	led_init();
	while (1)
	{

		for (uint16_t i = 0; i < ROWS * ROWS; i++)
			l[i] = 0;
		uint8_t mode_action = 0;
		mode = mode % 2;
		mode_state = mode_state % 3;
		if (led_button)
		{
			l2led(); // immediately blank display
			tick();	 // debounce push
			pushcount = 0;
			while (led_button)
			{
				tick();
				if (!(++pushcount))
					pushcount--;

			}
			if (pushcount > 200)
				powerdown();
			else if (pushcount > 50)
			{
				mode++;
				mode_state = 0;
				mode_num = 0;
				setup_mode(mode);
			}
			else if (pushcount > 5)
				mode_action = 1;
			tick(); // debounce release
		}
		if (mode == 0 || mode == 1)
		{
			if (mode_state == 1)
				l[16 * 8 + 6] = 15;
			// stopwatch and timer work using the same principle
			// timer just displays 99-num

			if (mode == 0)
			{
				show_num(mode_num);
			}
			else
			{
				show_num(99 - mode_num);
			}

			if (mode_action == 1 && mode_state == 2)
			{
				mode_num = 0;
			}

			if (mode_action == 1 || (mode == 0 && mode_num == 99 && mode_state == 1))
			{
				if (mode_state++ == 0)
				{
					start_timer();
				}
			}
		}
		tick();
		show_mode(mode);
		l2led();
	}
}

void setup(void)
{
	led_init();
	sei();
}

void tick(void)
{
	uint8_t tmp;
	tmp = led_tick;
	while (tmp == led_tick)
		;
}

void powerdown(void)
{
	cli();
	TIMSK = 0x00; // disable timer 1 compare A interrupt
	DDRA = 0x00;
	DDRB = 0x00;
	DDRC = 0x00;
	DDRD = 0x00;
	DDRE = 0x00;
	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0x04;
	PORTE = 0x00;
	MCUCR &= ~0x03; // select low level for INT0
	GIFR = 0x00;	// clear pending interrupts
	GICR = 0x40;	// enable INT0
	sei();
	MCUCR |= 0x20; // enable sleep
	asm volatile("sleep\n");
}