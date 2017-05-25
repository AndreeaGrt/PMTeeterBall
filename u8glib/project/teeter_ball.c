#include "u8g.h"
#include "mpu6050.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#define PAD_INCREMENT	10
#define NO_LINES         4
#define NO_HOLES        13
#define RAD              2
#define RAD_HOLE         3
#define DIAM       	     5
#define DIST            16 //(RAD + RAD)^2
#define MAX_X		   127
#define MIN   			 0
#define MAX_Y  			63
#define INIT_X  		 4
#define INIT_Y  		 4
#define MAX_LVL          3
#define NUM_NOTE         8

/* LCD-ul si accelerometrul legate ca in schema, 
 *buton la PB0 si buzzer la PB1 
 */

/* Bitmap-ul de la inceputul jocului */
const uint8_t ball[128] PROGMEM = {  
 0x00 , 0x0F , 0xF0 , 0x00 , 0x00 , 0x3F , 0xFC , 0x00 , 
 0x00 , 0xFF , 0x0F , 0x00 , 0x00 , 0xF8 , 0x07 , 0x80 ,
 0x07 , 0xC0 , 0x3F , 0xE0 , 0x0F , 0x80 , 0xFF , 0xF0 , 
 0x0F , 0x03 , 0xFF , 0xF0 , 0x1E , 0x07 , 0xFF , 0xF8 ,
 0x3C , 0x1F , 0xFF , 0xFC , 0x38 , 0x3F , 0xFF , 0xFC , 
 0x78 , 0x3F , 0xFF , 0xFE , 0x70 , 0x7F , 0xFF , 0xFE ,
 0x61 , 0xFF , 0xFF , 0xFE , 0xE1 , 0xFF , 0xFF , 0xFF ,
 0xC7 , 0xFF , 0xFF , 0xFF , 0xCF , 0xFF , 0xFF , 0xFF ,
 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 
 0xFF , 0xFF , 0xFF , 0xFF , 0x7F , 0xFF , 0xFF , 0xFE ,
 0x7F , 0xFF , 0xFF , 0xFE , 0x7F , 0xFF , 0xFF , 0xFE , 
 0x3F , 0xFF , 0xFF , 0xFC , 0x3F , 0xFF , 0xFF , 0xFC ,
 0x1F , 0xFF , 0xFF , 0xF8 , 0x0F , 0xFF , 0xFF , 0xF0 , 
 0x0F , 0xFF , 0xFF , 0xF0 , 0x07 , 0xFF , 0xFF , 0xE0 ,
 0x01 , 0xFF , 0xFF , 0x00 , 0x00 , 0xFF , 0xFF , 0x00 , 
 0x00 , 0x3F , 0xFC , 0x00 , 0x00 , 0x03 , 0xE0 , 0x00
};

u8g_t u8g;
double x = 0, y = 0, z = 0, unused;
unsigned int x_st[NO_LINES]  = { 0,  25,  0,  25};
unsigned int x_end[NO_LINES] = {90, 127, 88, 127};
unsigned int y_st[NO_LINES]  = {17,  34, 44,  54};
unsigned int y_end[NO_LINES] = {17,  34, 44,  54};
unsigned int frecv_nota[NUM_NOTE] = {262, 294, 330, 349, 392, 440, 494, 523};
unsigned int hole_x[NO_HOLES] = {35, 75, 112, 120, 100, 80, 37, 
								 10, 92, 121,  7, 15, 120};
unsigned int hole_y[NO_HOLES] = { 6, 10,   6,  29,  29, 22, 30, 
								 22, 43,  49, 52, 59,  59};
// win = 0, bila a cazut; win = 2, jocul e castigat; win = 1, nimic;
unsigned char win = 1; 
unsigned int pos_x = INIT_X;
unsigned int pos_y = INIT_Y;

void u8g_setup(void)
{  
	u8g_InitSPI(&u8g, &u8g_dev_st7920_128x64_sw_spi, 
		        PN(1, 7), PN(1, 5), PN(1, 4), U8G_PIN_NONE, U8G_PIN_NONE);
}

void sys_init(void)
{
  /* minimal prescaler (max system speed) */
  CLKPR = 0x80;
  CLKPR = 0x00;
}

void draw_frame(void)
{
	u8g_DrawLine(&u8g, MIN, MIN, MIN, MAX_Y);
	u8g_DrawLine(&u8g, MIN, MIN, MAX_X, MIN);
	u8g_DrawLine(&u8g, MIN, MAX_Y, MAX_X, MAX_Y);
	u8g_DrawLine(&u8g, MAX_X, MIN, MAX_X, MAX_Y);
}

/* Ecanul de start */
void draw_start(void)
{
	/* rotesc ecranul cu 180 de grade: 
	 * datorita pozitiei in care l-am lipit 
	 */
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

/*Ecranul de sfarsit*/
void draw_end(int x)
{
	u8g_SetRot180(&u8g);
	u8g_SetFont(&u8g, u8g_font_5x8r);
	draw_frame();
	
	u8g_SetScale2x2(&u8g);
	if (x)
		u8g_DrawStr(&u8g, 14, 18, "You win!");
	u8g_UndoScale(&u8g);

	u8g_DrawStr(&u8g, 15, 55, "Press button to play");
	u8g_DrawStr(&u8g, 50, 61, "again");
}


/* Genereaza o sunetul folosind buzzer-ul. */
void falling_sound(void) 
{
	int nota, perioada_nota;
	int i, j;
	for (nota = 0;  nota < 8; nota++) {
		for (j = 0 ; j < 10; j++) {
			perioada_nota = 1000000 / frecv_nota[nota];
			
			PORTB |= (1 << PB1);
			for (i = 0; i < perioada_nota / 2; ++i)
				_delay_us(1);

			PORTB &= ~(1 << PB1);
			for (i = 0; i < perioada_nota / 2; ++i)
				_delay_us(1);
		}
	}
}

/* Modificarea pozitiei bilei, determinata de
 * miscarea accelerometrului
 */
void play(void) 
{
	int val_x, val_y, aux1, aux2;;
	unsigned char i;

	mpu6050_getConvData(&x, &y, &z, &unused, &unused, &unused);
	val_x = y * PAD_INCREMENT;
	val_y = x * PAD_INCREMENT;

	if (!win) {
		/* atunci dupa ce cade intr-o gaura, 
		 * bila va reincepe de pe pozitia initiala 
		 */
		pos_x = INIT_X;
		pos_y = INIT_Y;
		win = 1;
		falling_sound();
		
	} else {
		for (i = 0; i < NO_LINES; i++) {
			/* Pentru segmente verticale
			if (pos_y >= y1[i] && pos_y <= y2[i]) {
				if (val_x > 0  && pos_x - x1[i] <= RAD + val_x)
					val_x =  pos_y - x1[i] - RAD - 1;
				else if (val_x < 0  && x1[i] -  pos_x <= RAD - val_x)
					val_x = RAD + 1 - (x1[i] - pos_x);
			}*/
			/* Inersectia cu segementele orizontale */
			if (pos_x >= x_st[i] && pos_x <= x_end[i]) {
				if (val_y > 0  && pos_y - y_st[i] <= RAD + val_y)
					val_y =  pos_y - y_st[i] - RAD - 1;
				else if (val_y < 0  && y_st[i] -  pos_y <= RAD - val_y)
					val_y = RAD + 1 - (y_st[i] - pos_y);
			}
		}

		/* Intersectia cu gaurile */
		for (i = 0 ; i < NO_HOLES - 1; i++){
			aux1 = pos_x - hole_x[i];
			aux2 = pos_y - hole_y[i];
			if (aux1 * aux1 + aux2 * aux2 <= DIST) {
				pos_x = hole_x[i];
				pos_y = hole_y[i];
				win = 0;
				return;
			}
		}

		/* destinatia finala */
		aux1 = pos_x - hole_x[i];
		aux2 = pos_y - hole_y[i];
		if (aux1 * aux1 + aux2 * aux2 <= DIST) {
				win = 2;
				return;
		}

		/* intersectia cu "rama"*/
		aux1 = pos_x - val_x;
		aux2 = pos_y - val_y;
	    if (val_x > 0 && aux1 < DIAM)
			val_x =  pos_x - RAD - 1;
		else if (val_x < 0  && aux1 > MAX_X - DIAM)
			val_x = pos_x  - (MAX_X - RAD - 1);
		
		if (val_y > 0  && aux2 < DIAM)
			val_y =  pos_y - RAD - 1;
		else if (val_y < 0  && aux2 >= MAX_Y - DIAM)
			val_y = pos_y  - (MAX_Y - RAD - 1);

		pos_x -= val_x;
	    pos_y -= val_y;
	}
}

/* Marcheaza destinatia finala */
void draw_X(void)
{
	unsigned char last = NO_HOLES - 1;

	u8g_DrawLine(&u8g, hole_x[last] - 1, hole_y[last] - 1, 
					   hole_x[last] + 1, hole_y[last] + 1);
	u8g_DrawLine(&u8g, hole_x[last] - 1, hole_y[last] + 1, 
					   hole_x[last] + 1, hole_y[last] - 1);
}

void draw_level(void)
{
	unsigned char i;
	
	u8g_SetRot180(&u8g);
	draw_frame();
	
	for (i = 0; i < NO_LINES; ++i)
		u8g_DrawLine(&u8g, x_st[i], y_st[i], x_end[i], y_st[i]);	
	
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
	int i = 1; 
	/* Setez pinul PB0 ca intrare : buton */
	DDRB &= ~(1 << PB0);
	PORTB |= (1 << PB0);
	/* Setez pinul PB01 ca iesire: buzzer */
	DDRB |= (1 << PB1);

	/* Pagina de inceput */
	for(;;) {  
		u8g_FirstPage(&u8g);
		do
		{ 
		 draw_start();
		} while ( u8g_NextPage(&u8g) );
		u8g_Delay(10);

		/* La apasarea butonului, incepe jocul*/
		if ((PINB & (1 << PB0)) == 0)
		 	break;
	}

	for(;;) { 
		i = 1;

		/* Jocul */ 
		for(;;) {  
			u8g_FirstPage(&u8g);
			do
			{
				draw_level();
			} while ( u8g_NextPage(&u8g) );
			play();	
			if (win == 2)  // la castig
				break;
		}

		/* Pagina de final */
		for(;;) {  
			u8g_FirstPage(&u8g);
			do
			{ 
			 draw_end(i);
			} while ( u8g_NextPage(&u8g) );
			u8g_Delay(10);
			i ^= 1;
			win = 0;
			/* La apasarea butonului reincepe jocul */
			if ((PINB & (1 << PB0)) == 0)
				break;
		}
	}
}
