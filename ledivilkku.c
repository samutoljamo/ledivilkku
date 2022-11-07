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

extern const char animation[];
uint8_t animationsequence = 0;

PGM_P a_ptr;  // pointer to animation script in PROGMEM
uint16_t a_w; // animation wait counter
uint8_t a_e;  // animation selected effect

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

show_mode(uint8_t m)
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
/*
while(1)
	{
	if (led_button)
	   {
	   led_init();   // immediately blank display
	   tick();       // debounce push
	   pushcount=0;
	   while (led_button)
		  {
		  tick();
		  if (!(++pushcount)) pushcount--;
		  }
	   if (pushcount>50) powerdown();
	   tick();    // debounce release
	   animationsequence++;
	   setanimation();
	   }
	if (1)
	   {
	   animate();
	   }
	else
	   {
	   matrix();
	   }
	tick();
	}
}



void animate(void)
{
uint8_t a_b;
uint8_t a_s;
while (a_w==0) // loop until we reach wait statement - or are already waiting
   {
   a_b=pgm_read_byte_near(a_ptr++);
   if ((a_b=='x') || (a_b==0)) // end reached - reset and wait for one cycle (to defend against empty lists)
	  {
	  setanimation();
	  //a_ptr=animation;
	  return;
	  }
   switch(a_b)
	  {
	  case 'e':
		 a_e=animate_parsevalue(2);
		 break;
	  case 's':
		 l[animate_parsevalue(2)]=a_e;
		 break;
	  case 'a':
		 for (uint16_t i=0;i<256;i++) l[i]=a_e;
		 break;
	  case 'p':
		 a_s=animate_parsevalue(2);
		 uint8_t amount = a_s&0x0f;
		 if (a_s&0x10) for (uint8_t i=0;i<16;i++) // shift left
			{
			uint8_t offset = i<<4;
			for (uint8_t j=0;j<16-amount;j++)
			   {
			   l[offset+j]=l[offset+j+amount];
			   }
			}
		 amount = (a_s&0x0f)<<4;
		 if (a_s&0x20) for (int16_t i=0;i<256-amount;i++) l[i]=l[i+amount];// shift up
		 amount = a_s&0x0f;
		 if (a_s&0x40) for (uint8_t i=0;i<16;i++) // shift right
			{
			uint8_t offset = i<<4;
			for (uint8_t j=15;j>=amount;j--)
			   {
			   l[offset+j]=l[offset+j-amount];
			   }
			}
		 amount = (a_s&0x0f)<<4;
		 if (a_s&0x80) for (int16_t i=255;i>=amount;i--) l[i]=l[i-amount];// shift down
		 break;
	  case 'w':
		 a_w=animate_parsevalue(4);
		 break;
	  default:	// should never happen - reset animation to the start of current sequence
		 setanimation();
		 //a_ptr=animation;
		 break;
	  }
   }
if (a_w!=0xffff) a_w--; // 0xffff equals STOP
if (led_tick&0x03) return; // run autoanimation only every 4th tick
uint8_t i=0;
do {
   uint8_t a_tmp=l[i];
   if (a_tmp&0x10) // automatic animation command for this led
	  {
	  uint8_t a_c,a_d;
	  a_d=a_tmp&0x0f;
	  a_c=a_tmp&0xf0;
	  if (a_c&0x20)
		 {
		 a_d++;
		 if (a_d&0xf0)
			{
			a_d=0x0f;
			if (a_c&0x40)
			   {
			   a_c^=0x20;
			   }
			else
			   {
			   a_c&=~0x10;
			   }
			}
		 }
	  else
		 {
		 a_d--;
		 if (a_d&0xf0)
			{
			a_d=0x00;
			if (a_c&0x40)
			   {
			   a_c^=0x20;
			   }
			else
			   {
			   a_c&=~0x10;
			   }
			}
		 }
	  l[i]=a_c|a_d;
	  }
   i++;
   }
while (i!=0); // loop all 256 values
l2led();
}



void setanimation(void)
{
uint8_t seqno=0;
uint8_t a_b;
a_ptr=animation;
while (seqno!=animationsequence)
   {
   a_b=pgm_read_byte_near(a_ptr++);
   if (a_b==0) // end of program - jump to beginning
	  {
	  animationsequence=0;
	  a_ptr=animation;
	  break;
	  }
   if (a_b=='x') seqno++;
   }
a_w=0;
}



uint16_t animate_parsevalue(uint8_t digits)
{
uint16_t result=0,c;
while (digits--)
   {
   c=pgm_read_byte_near(a_ptr++);
   result<<=4;
   result+=animate_hex2dec(c);
   }
return(result);
}



uint8_t animate_hex2dec(uint8_t character)
{
if ((character>='0')&&(character<='9')) return(character-'0');
if ((character>='a')&&(character<='f')) return(character+10-'a');
if ((character>='A')&&(character<='F')) return(character+10-'A');
return(0); // should never happen
}
*/

void setup(void)
{
	led_init();
	a_ptr = &animation[0];
	a_w = 0;
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

/*
void matrix(void)
{
while (led_phase==3);
while (led_phase!=3);
uint8_t *ptr=l;
for (uint8_t i=255;i>255-16;i--)
   {
   uint8_t tmp;
   tmp=ptr[i];
   if (tmp)
	  {
	  ptr[i]=tmp-1;
	  }
   }
for (uint8_t i=255-16;i!=255;i--) // careful with sign
   {
   uint8_t tmp;
   tmp=ptr[i];
   if (tmp)
	  {
	  if (ptr[i+16]<tmp)
		 {
		 ptr[i+16]=tmp;
		 }
	  ptr[i]=tmp-1;
	  }
   }
for (uint8_t i=0;i<16;i++) if ((rand()&0x1f)==0x1f)
   {
   rand();rand();rand();rand();rand();rand();rand();rand();
   l[i]=rand()&0x0f;
   }
for (uint8_t i=0;i<7;i++) tick();
l2led();
}*/

/*
while(1)
	{
	while (led_phase==3);
	while (led_phase!=3);
	uint8_t *ptr=l;
	for (uint8_t i=255;i>255-16;i--)
	   {
	   uint8_t tmp;
	   tmp=ptr[i];
	   if (tmp)
		  {
		  ptr[i]=tmp-1;
		  }
	   }
	for (uint8_t i=255-16;i!=255;i--) // careful with sign
	   {
	   uint8_t tmp;
	   tmp=ptr[i];
	   if (tmp)
		  {
		  if (ptr[i+16]<tmp)
			 {
			 ptr[i+16]=tmp;
			 }
		  ptr[i]=tmp-1;
		  }
	   }
	for (uint8_t i=0;i<16;i++) if ((rand()&0x1f)==0x1f)
	   {
	   rand();rand();rand();rand();rand();rand();rand();rand();
	   l[i]=rand()&0x0f;
	   }
	for (uint8_t i=0;i<20;i++) tick();
	l2led();
	}
	*/
/*
// nice tests

for (uint8_t i=0;i<16;i++) led_set(15-i,i,1);
for (uint8_t i=0;i<16;i++) led_set(i,i,1);

while(1)
	{
	while (led_phase==3);
	while (led_phase!=3);
	uint8_t *ptr=l_port;
	for (int8_t i=127-8;i>=0;i--) // careful with sign
	   {
	   ptr[i+8]=ptr[i];
	   }
	for (uint8_t i=0;i<16;i++) led_set(i,0,((rand()&0x0f)==15) ? (rand()&0x0f) : 0);
	for (uint8_t i=0;i<16;i++) tick();
	}

while(1)
	{
	led_set(rand()&0x0f,rand()&0x0f,rand()&0x01);
	tick();
	}

while(1)
	{
	for (uint8_t i=15;i>0;i--) led[i]=led[i-1];
	for (uint8_t i=0;i<16;i++) led_set(i,0,rand()&0x01);
	for (uint8_t i=0;i<32;i++) tick();
	}

*/
