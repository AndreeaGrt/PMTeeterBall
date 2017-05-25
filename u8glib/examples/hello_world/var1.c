#include "u8g.h"
#include "mpu6050.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#define PAD_INCREMENT	8
#define NO_LINES    4
#define NO_HOLES    12
#define RAD          2
#define RAD_HOLE      3
#define DIAM       	5
#define DIST       16//(RAD + RAD) * (RAD + RAD);
#define MAX_X 127
#define MIN   0
#define MAX_Y  63
#define INIT_X  4
#define INIT_Y  4

const uint8_t  ball[128] PROGMEM = {  
 0x00 , 0x0F , 0xF0 , 0x00 , 
 0x00 , 0x3F , 0xFC , 0x00, 
 0x00 , 0xFF , 0x0F , 0x00 , 
 0x00 , 0xF8 , 0x07 , 0x80,
 0x07 , 0xC0 , 0x3F , 0xE0 , 
 0x0F , 0x80 , 0xFF , 0xF0 , 
 0x0F , 0x03 , 0xFF , 0xF0 , 
 0x1E , 0x07 , 0xFF , 0xF8,
 0x3C , 0x1F , 0xFF , 0xFC , 
 0x38 , 0x3F , 0xFF , 0xFC , 
 0x78 , 0x3F , 0xFF , 0xFE , 
 0x70 , 0x7F , 0xFF , 0xFE,
 0x61 , 0xFF , 0xFF , 0xFE , 
 0xE1 , 0xFF , 0xFF , 0xFF ,
 0xC7 , 0xFF , 0xFF , 0xFF , 
 0xCF , 0xFF , 0xFF , 0xFF,
 0xFF , 0xFF , 0xFF , 0xFF ,
 0xFF , 0xFF , 0xFF , 0xFF , 
 0xFF , 0xFF , 0xFF , 0xFF ,
 0x7F , 0xFF , 0xFF , 0xFE,
 0x7F , 0xFF , 0xFF , 0xFE , 
 0x7F , 0xFF , 0xFF , 0xFE , 
 0x3F , 0xFF , 0xFF , 0xFC , 
 0x3F , 0xFF , 0xFF , 0xFC,
 0x1F , 0xFF , 0xFF , 0xF8 , 
 0x0F , 0xFF , 0xFF , 0xF0 , 
 0x0F , 0xFF , 0xFF , 0xF0 , 
 0x07 , 0xFF , 0xFF , 0xE0,
 0x01 , 0xFF , 0xFF , 0x00 , 
 0x00 , 0xFF , 0xFF , 0x00 , 
 0x00 , 0x3F , 0xFC , 0x00 , 
 0x00 , 0x03 , 0xE0 , 0x00
};

u8g_t u8g;
double x = 0 , y = 0 , z = 0, unused;
unsigned int x1[NO_LINES] = { 0,  25,  0,  25};
unsigned int x2[NO_LINES] = {90, 127, 88, 127};
unsigned int y1[NO_HOLES] = {17,  34, 44,  54};
//unsigned int y2[4] = {17,  34, 44,  54};

unsigned int hole_x[NO_HOLES]={35, 75, 112, 120, 100, 80, 37, 10, 92, 121,  7, 120};
unsigned int hole_y[NO_HOLES]={ 6, 10,   6,  29,  29, 22, 30, 22, 43,  49, 54,  59};

unsigned char win = 1;

void u8g_setup(void)
{  
	u8g_InitSPI(&u8g, &u8g_dev_st7920_128x64_sw_spi, PN(1, 7), PN(1, 5), PN(1, 4), U8G_PIN_NONE, U8G_PIN_NONE);
}

void sys_init(void)
{
//#if defined(__AVR__)
  /* select minimal prescaler (max system speed) */

  CLKPR = (1 << 7);
//#endif
}

unsigned int pos_x = INIT_X;
unsigned int pos_y = INIT_Y;

void draw_start(void){
	u8g_SetRot180(&u8g);
	u8g_SetFont(&u8g, u8g_font_5x8r);
	u8g_DrawStr(&u8g, 10, 55, "Press button to start ");
	u8g_DrawBitmapP(&u8g, 70, 0, 4, 32, ball);
	u8g_DrawLine(&u8g, 3, 45, 3, 60);
	u8g_DrawLine(&u8g, 3, 60, 123, 60);
	u8g_DrawLine(&u8g, 123, 60, 123, 45);
	u8g_DrawLine(&u8g, 123, 45, 3, 45);

	u8g_SetScale2x2(&u8g);
	u8g_DrawStr(&u8g, 4, 10, "Teeter");
	u8g_DrawStr(&u8g, 10, 20, "BALL");
	u8g_UndoScale(&u8g);
}

void draw_end(void){
	u8g_SetRot180(&u8g);
	u8g_SetFont(&u8g, u8g_font_5x8r);
	draw_frame();
	u8g_SetScale2x2(&u8g);
	u8g_DrawStr(&u8g, 14, 18, "You win!");
	u8g_UndoScale(&u8g);
	u8g_DrawStr(&u8g, 15, 55, "Press button to play");
	u8g_DrawStr(&u8g, 50, 61, "again");
}

void draw_frame(void){
	u8g_DrawLine(&u8g, MIN, MIN, MIN, MAX_Y);
	u8g_DrawLine(&u8g, MIN, MIN, MAX_X, MIN);
	u8g_DrawLine(&u8g, MIN, MAX_Y, MAX_X, MAX_Y);
	u8g_DrawLine(&u8g, MAX_X, MIN, MAX_X, MAX_Y);
}

void play(void) {
	int valx, valy;
	unsigned char i;
	int val1, val2;

	mpu6050_getConvData(&x, &y, &z, &unused, &unused, &unused);
	valx = y * PAD_INCREMENT;
	valy = x * PAD_INCREMENT;

	if (!win) {
		pos_x = INIT_X;
		pos_y = INIT_Y;
		win = 1;
	} else {
		for (i = 0; i < NO_LINES; i++) {
			/*if (pos_y >= y1[i] && pos_y <= y2[i]) {
				if (valx > 0  && pos_x - x1[i] <= RAD + valx)
					valx =  pos_y - x1[i] - RAD - 1;
				else if (valx < 0  && x1[i] -  pos_x <= RAD - valx)
					valx = RAD + 1 - (x1[i] - pos_x);
			}*/
			if (pos_x >= x1[i] && pos_x <= x2[i]) {
				if (valy > 0  && pos_y - y1[i] <= RAD + valy)
					valy =  pos_y - y1[i] - RAD - 1;
				else if (valy < 0  && y1[i] -  pos_y <= RAD - valy)
					valy = RAD + 1 - (y1[i] - pos_y);
			}
		}
		for (i = 0 ; i < NO_HOLES - 1; i++){
			val1 = pos_x - hole_x[i];
			val2 = pos_y - hole_y[i];
			if (val1 * val1 + val2 * val2 <= DIST) {
				pos_x = hole_x[i];
				pos_y = hole_y[i];
				win = 0;
				return;
			}
		}

		val1 = pos_x - hole_x[i];
		val2 = pos_y - hole_y[i];
		if (val1 * val1 + val2 * val2 <= DIST) {
				win = 2;
				return;
		}

		val1 = pos_x - valx;
		val2 = pos_y - valy;
	    if (valx > 0 && val1 < DIAM)
			valx =  pos_x - RAD - 1;
		else if (valx < 0  && val1 > MAX_X - DIAM)
			valx = pos_x  - (MAX_X - RAD - 1);
		
		if (valy > 0  && val2 < DIAM)
			valy =  pos_y - RAD - 1;
		else if (valy < 0  && val2 >= MAX_Y - DIAM)
			valy = pos_y  - (MAX_Y - RAD - 1);

		pos_x -= valx;
	    pos_y -= valy;
	}
}

void draw_X(void)
{
	unsigned char last = NO_HOLES - 1;

	u8g_DrawLine(&u8g, hole_x[last] - 1, hole_y[last] - 1, 
					   hole_x[last] + 1, hole_y[last] + 1);
	u8g_DrawLine(&u8g, hole_x[last] - 1, hole_y[last] + 1, 
					   hole_x[last] + 1, hole_y[last] - 1);
}

void draw_level1(void){
	u8g_SetRot180(&u8g);
	draw_frame();
	unsigned char i;
	for (i = 0; i < NO_LINES; ++i)
		u8g_DrawLine(&u8g, x1[i], y1[i], x2[i], y1[i]);	
	
	for ( i = 0; i < NO_HOLES ; i++)
		u8g_DrawCircle(&u8g, hole_x[i], hole_y[i], RAD_HOLE, U8G_DRAW_ALL);

	draw_X();
	u8g_DrawDisc(&u8g, pos_x, pos_y, RAD, U8G_DRAW_ALL);
}


int main(void)
{
  sys_init();
  mpu6050_init();
  u8g_setup();
	
//	int level = 0;	
	/* Setez pinul PB0 ca intrare. */
	DDRB &= ~(1 << PB0);
	/* Activez rezistenta de pull-up pentru pinul PB0. */
	PORTB |= (1 << PB0);
	/* Setez pinul PB0 ca iesire. */
	DDRB |= (1 << PB1);


	for(;;) {  
		u8g_FirstPage(&u8g);
		do
		{ 
		 draw_start();
		} while ( u8g_NextPage(&u8g) );
		u8g_Delay(10);
		if ((PINB & (1 << PB0)) == 0)
			break;
	}
	for(;;) {  
		for(;;) {  
			u8g_Delay(10);
			u8g_FirstPage(&u8g);
			do
			{
				draw_level1();
			} while ( u8g_NextPage(&u8g) );
			play();	
			if (win == 2) 
				break;
		}
		for(;;) {  
			u8g_FirstPage(&u8g);
			do
			{ 
			 draw_end();
			} while ( u8g_NextPage(&u8g) );
			u8g_Delay(10);
			if ((PINB & (1 << PB0)) == 0)
				break;
		}
	}
}
