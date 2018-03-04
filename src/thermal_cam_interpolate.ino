/***************************************************************************
 Firmware for a ThermoCam

 Based on an example from Dean Miller, James DeVito & ladyada for Adafruit
 Industries.
 ***************************************************************************/

// ------------------begin ESP8266'centric----------------------------------
#define FREQUENCY     80                  // valid 80, 160

//
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}
// ------------------end ESP8266'centric------------------------------------

#include <Adafruit_GFX.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_AMG88xx.h>
#include <eeprom.h>
#include "OneButton.h"
#include "UrsADC.h"

#define EEPROM_SIZE 128
enum {
  EEP_VALID,
  EEP_CALV,
  EEP_NEXT = EEP_CALV + sizeof(double),
};

// define the button-pins
#define B_MODE  		0
#define B_MAXP 			10
#define B_MAXM 			12
#define B_MINP  		3
#define B_MINM  		1
#define O_PWR  			16

// define position and size of the battery symbol
#define BATT_X 21
#define BATT_Y  9
#define BATT_START_X	100
#define BATT_START_Y	5

// define voltage limit levels
#define VBATT_CRIT		3.5
#define VBATT_EMPT		3.35
#define VBATT_OFF		3.3

// a helper macro
#define min(a,b) ((a)<(b)?(a):(b))

//low range of the sensor (this will be blue on the screen)
int MINTEMP = 15;

//high range of the sensor (this will be red on the screen)
int MAXTEMP = 35;

// the yellow battery symbol
const unsigned short bat_critical[] = {
0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41,   // 0x0010 (16) pixels
0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41,   // 0x0020 (32) pixels
0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0030 (48) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFE41, 0xFE41, 0xFFFF, 0xFE41,   // 0x0040 (64) pixels
0xFE41, 0xFFFF, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0050 (80) pixels
0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0060 (96) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF,   // 0x0070 (112) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41,   // 0x0080 (128) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0090 (144) pixels
0xFE41, 0xFE41, 0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41,   // 0x00A0 (160) pixels
0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41,
0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFE41, 0xFFFF, 0xFFFF, 0xFFFF,
};

// the red battery symbol
const unsigned short bat_empty[] = {
0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4,   // 0x0010 (16) pixels
0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4,   // 0x0020 (32) pixels
0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xE8E4, 0xE8E4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0030 (48) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xE8E4, 0xE8E4, 0xFFFF, 0xE8E4,   // 0x0040 (64) pixels
0xE8E4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0050 (80) pixels
0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0060 (96) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0070 (112) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4,   // 0x0080 (128) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0090 (144) pixels
0xE8E4, 0xE8E4, 0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4,   // 0x00A0 (160) pixels
0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4,
0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xE8E4, 0xFFFF, 0xFFFF, 0xFFFF,
};

//the colormap for temperatures
const uint16_t camColors[] = {0x480F,
0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,
0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,
0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,
0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,
0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,
0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,
0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,
0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,
0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,
0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,
0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,
0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,
0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800};


#define AUTO_OFF_TIMEOUT	300000L		// shut down after 5 minutes without action
#define AMG_COLS 			8			// 8 columns of thermo pixel
#define AMG_ROWS 			8			// 8 rows of thermo pixel
#define INTERPOLATED_COLS 	32			// 32 temperature points per column on TFT
#define INTERPOLATED_ROWS 	32			// 32 temperature points per row on TFT
#define RAWMODE				0x02		// bitmask for not interpolated display
#define MANUAL				0x01		// bitmask for manual temperature ranges

bool blink_flag = 			false;		// common flag for blink functions
bool volt_flag = 			false;		// voltage display mode active
bool cal_flag = 			false;		// calibration mode active
double cal_val, cal_step;				// help values for calibration mode
int cal_pos;
double  VBATT_CORR = 		0.0051939453125;	// scaling factor for voltage measurement
double min_drag = 			999.0;		// lower value of trailing pointers
double max_drag = 		   -999.0;		// higher value of trailing pointers
uint32_t blink_millis = 	0;			// timer for blink functions
uint32_t auto_off_millis;				// timer for automatic shutdown
float pixels[AMG_COLS * AMG_ROWS];
float dest_2d[INTERPOLATED_ROWS * INTERPOLATED_COLS];
volatile int modeflag = 	0;
volatile int draginit = 	1;

TFT_eSPI tft = TFT_eSPI();       		// Invoke custom library
Adafruit_AMG88xx amg;					// create sensor object
OneButton b_mode(B_MODE, true);			// create button objects
OneButton b_maxp(B_MAXP, true);
OneButton b_maxm(B_MAXM , true);
OneButton b_minp(B_MINP, true);
OneButton b_minm(B_MINM, true);

// calculate the coordinates of given point index
float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);

// interpolate the 8x8 sensor array to 32x32 for TFT
void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, float *dest, uint8_t dest_rows, uint8_t dest_cols);

// draw the pixels regarding the measured temperatures
void drawpixels(float *p, uint8_t rows, uint8_t cols, uint8_t boxWidth, uint8_t boxHeight);

// read a double value from given address in EEPROM
double eep_read_double(int address)
{
	unsigned int i;
	double dval;
	unsigned char *cp = (unsigned char*)&dval;

	for(i = 0; i < sizeof(double); i++)
	{
		*cp = EEPROM.read(address + i);
		++cp;
	}
	return dval;
}

// write a double value to given address in EEPROM
void eep_write_double(int address, double value)
{
	unsigned int i;
	double dval = value;
	unsigned char *cp = (unsigned char*)&dval;

	for(i = 0; i < sizeof(double); i++)
	{
		EEPROM.write(address + i, *cp);
		++cp;
	}
	EEPROM.commit();
}

// measure the battery voltage
double read_batt(void)
{
	return ((double)UrsAdc.read() * VBATT_CORR);
}

// doubleclick function of MIN-
void toggle_cal_flag(void)
{
	if(cal_flag ^= 1)
	{
		volt_flag = 0;
		cal_val = read_batt();
		cal_step = 1.0;
		cal_pos = 0;
		tft.setTextSize(2);
		tft.fillScreen(TFT_BLACK);
		tft.setCursor(12, tft.height() / 2 - 49 );
		tft.print("measured");
		tft.setCursor(12, tft.height() / 2 - 32 );
		tft.print("Voltage:");
	}
}

// doubleclick function of MIN+
void toggle_volt_flag(void)
{
	if(!cal_flag)
	{
		if(volt_flag ^= 1)
		{
			tft.setTextSize(2);
			tft.setCursor(12, tft.height() / 2 - 32 );
			tft.fillScreen(TFT_BLACK);
			tft.print("Voltage:");
		}
		auto_off_millis = millis();
	}
}

// click function of SET
void init_dragging(void)
{
	draginit = 1;
	auto_off_millis = millis();
}

// longpress function of SET
void next_mode(void)
{
	if(cal_flag)
	{
		VBATT_CORR = cal_val / (double)UrsAdc.read();
		eep_write_double(EEP_CALV, VBATT_CORR);
		cal_flag = 0;
	}
	else
	{
		++modeflag;
		modeflag %= 3;
		if (modeflag & MANUAL)
		{
			max_drag = (int)max_drag;
			min_drag = (int)min_drag;
		}
	}
	auto_off_millis = millis();
}

// click function of MAX+
void do_maxp(void)
{
	if(cal_flag)
	{
		if((cal_val + cal_step) <= 5.5)
			cal_val += cal_step;
		blink_millis = 0;
	}
	else
	{
		if (modeflag & MANUAL)
			++max_drag;
	}
	auto_off_millis = millis();
}

// click function of MAX-
void do_maxm(void)
{
	if(cal_flag)
	{
		if((cal_val - cal_step) >= 3.0)
			cal_val -= cal_step;
		blink_millis = 0;
	}
	else
	{
		if (modeflag & MANUAL)
			if((max_drag - min_drag) > 6)
				--max_drag;
	}
	auto_off_millis = millis();
}

// click function of MIN+
void do_minp(void)
{
	if(cal_flag)
	{
		if(cal_pos < 3)
		{
			cal_pos++;
			cal_step /= 10.0;
		}
		blink_millis = 0;
	}
	else
	{
		if (modeflag & MANUAL)
			if((max_drag - min_drag) > 6)
				++min_drag;
	}
	auto_off_millis = millis();
}

// click function of MIN-
void do_minm(void)
{
	if(cal_flag)
	{
		if(cal_pos > 0)
		{
			cal_pos--;
			cal_step *= 10.0;
		}
		blink_millis = 0;
	}
	else
	{
		if (modeflag & MANUAL)
			--min_drag;
	}
	auto_off_millis = millis();
}

// doubleclick function of SET and common shutdown
void power_off(void)
{
	digitalWrite(O_PWR, LOW);
}

// show the battery symbol if necessary
void show_batt(void)
{
	int i, j;
	unsigned short *bptr;
	uint32_t last_millis = millis();
	static double voltage;

	voltage = read_batt();
	if((last_millis - blink_millis) > 1000)
	{
		blink_flag ^= 1;
		blink_millis = last_millis;
	}
	if(voltage < VBATT_CRIT)
	{
		bptr = (unsigned short*)bat_critical;
		if(voltage < VBATT_EMPT)
			bptr = (unsigned short*)bat_empty;
		if(voltage < VBATT_OFF)
			power_off();
		if(blink_flag)
		{
			for(j = 0; j < BATT_Y; j++)
			{
				for(i = 0; i < BATT_X; i++)
				{
					if(*bptr != TFT_WHITE)
						tft.drawPixel(BATT_START_X + i, BATT_START_Y + j, (unsigned short)*bptr);
					++bptr;
				}
			}
		}
	}
}

// initial settings
void setup()
{
	WiFi.forceSleepBegin();                  // turn off ESP8266 RF
	delay(1);                                // give RF section time to shutdown
	system_update_cpu_freq(FREQUENCY);

	pinMode(O_PWR, OUTPUT);
	digitalWrite(O_PWR, HIGH);				// hold the power switch

	Serial.begin(115200);
	Serial.println("\n\nAMG88xx Interpolated Thermal Camera!");

	EEPROM.begin(EEPROM_SIZE);
	if (EEPROM.read(EEP_VALID) == 0xAA)
	{
		VBATT_CORR = eep_read_double(EEP_CALV);
	}
	else
	{
		EEPROM.write(EEP_VALID, 0xAA);
		eep_write_double(EEP_CALV, VBATT_CORR);
	}

	// attach the button functions
	b_mode.attachClick(init_dragging);
	b_mode.attachLongPressStart(next_mode);
	b_mode.attachDoubleClick(power_off);
	b_maxp.attachClick(do_maxp);
	b_maxp.attachDuringLongPress(do_maxp);
	b_maxm.attachClick(do_maxm);
	b_maxm.attachDuringLongPress(do_maxm);
	b_minp.attachClick(do_minp);
	b_minp.attachDuringLongPress(do_minp);
	b_minp.attachDoubleClick(toggle_volt_flag);
	b_minm.attachClick(do_minm);
	b_minm.attachDuringLongPress(do_minm);
	b_minm.attachDoubleClick(toggle_cal_flag);

	// init the TFT
	tft.begin();
	tft.setRotation(2);
	tft.fillScreen(TFT_BLACK);

	auto_off_millis = millis();

	// init the sensor
	if (!amg.begin())
	{
		Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
		while (1)
		{
			delay(1);
		}
	}
	Serial.println("-- Thermal Camera Test --");
	Serial.end();		// no further serial output

	// reconfigure the button pins (serial output is disabled now)
	pinMode(B_MAXM, INPUT_PULLUP);
	pinMode(B_MAXP, INPUT_PULLUP);
	pinMode(B_MINM, INPUT_PULLUP);
	pinMode(B_MINP, INPUT_PULLUP);
	pinMode(B_MODE, INPUT_PULLUP);

	wdt_reset();
}

// the cyclic loop function
void loop()
{
	int i, j;
	float tval;
	volatile unsigned long amill;
	char txt[64];

	// read the sensor values
	amg.readPixels(pixels);

	// flip the array because of headfirst mounted sensor
	j = AMG_COLS * AMG_ROWS - 1;
	for(i=0; i < (AMG_COLS * AMG_ROWS / 2); i++)
	{
		tval = pixels[j];
		pixels[j] = pixels[i];
		pixels[i] = tval;
		--j;
	}

	// do the button functions
	b_mode.tick();
	b_maxp.tick();
	b_maxm.tick();
	b_minp.tick();
	b_minm.tick();

	amill = millis();

	if(cal_flag)								// calibration mode?
	{
		if((amill - blink_millis) > 500)
		{
			dtostrf(cal_val, 5, 3, txt);		// print calibration value
			if(blink_flag)
			{
				txt[cal_pos + ((cal_pos)?1:0)] = ' ';	// let active position blink
			}
			tft.setCursor(30, tft.height() / 2 - 5);
			tft.fillRect(30, tft.height() / 2 - 5, 60, 20, TFT_BLACK);
			tft.print(txt);						// write text to TFT
			blink_millis = amill;
			blink_flag ^= 1;
		}
	}
	else if(volt_flag)							// voltage display mode?
	{
		if((amill - blink_millis) > 500)
		{
			tft.setCursor(30, tft.height() / 2 - 5);
			dtostrf(read_batt(), 4, 2, txt);	// print voltage value
			strcat(txt, " V");
			tft.fillRect(30, tft.height() / 2 - 5, 46, 20, TFT_BLACK);
			tft.print(txt);						// write text to TFT
			blink_millis = amill;
			blink_flag ^= 1;
		}
	}
	else										// normal display mode
	{
		if (modeflag & RAWMODE)					// interpolation disabled?
		{
			unsigned char boxsize = min(tft.width() / AMG_COLS, tft.height() / AMG_ROWS);
			// draw pixels not interpolated
			drawpixels(pixels, AMG_ROWS, AMG_COLS, boxsize, boxsize);
		}
		else									// interpolation enabled?
		{
			unsigned char boxsize = min((uint8_t)(tft.width() / INTERPOLATED_COLS),
					(uint8_t)(tft.height() / INTERPOLATED_COLS));

			// draw interpolated pixels
			interpolate_image(pixels, AMG_ROWS, AMG_COLS, dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS);
			drawpixels(dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS, boxsize, boxsize);
		}
	}
	// test for necessary auto shutdown
	if((amill - auto_off_millis) > AUTO_OFF_TIMEOUT)
		if(read_batt() < 4.1)
			power_off();

	wdt_reset();
}

void drawpixels(float *p, uint8_t rows, uint8_t cols, uint8_t boxWidth, uint8_t boxHeight)
{
	int colorTemp;
	uint8_t colorIndex;
	unsigned long lmill = 0;
	double tmin = 100.0, tmax = 0;
	char txt[64];
	unsigned char xmax = 0, ymax = 0, tpos;

	// for all pixels
	for (int y = 0; y < rows; y++)
	{
		for (int x = 0; x < cols; x++)
		{
			// find minimal and maximal values
			float val = get_point(p, rows, cols, x, y);
			if (val >= MAXTEMP)
				colorTemp = MAXTEMP;
			else if (val <= MINTEMP)
				colorTemp = MINTEMP;
			else
				colorTemp = val;
			if (val > tmax)
			{
				tmax = val;
				xmax = x;
				ymax = y;
			}
			if ((val < tmin) && (val > 1.0))
				tmin = val;

			if (draginit)
			{
				draginit = 0;
				if (modeflag & MANUAL)
				{
					max_drag = (int)max_drag;
					min_drag = (int)min_drag;
				}
				else
				{
					min_drag = tmin;
					max_drag = tmax;
				}
			}
			else
			{
				if (!(modeflag & MANUAL))
				{
					if ((tmin > 0.0) && (tmin < min_drag))
						min_drag = tmin;
					if (tmax > max_drag)
						max_drag = tmax;
				}
			}
			// map temperature to color
			colorIndex = map(colorTemp, MINTEMP, MAXTEMP, 0, 255);
			colorIndex = constrain(colorIndex, 0, 255);
			// draw the colored rectangle
			tft.fillRect(boxWidth * x, boxHeight * y, boxWidth, boxHeight, camColors[colorIndex]);
		}
		// show battery symbol if necessary
		show_batt();
	}
	// refresh text every 750ms
	if ((millis() - lmill) > 750)
	{
		tpos = 105;
		lmill = millis();
		tft.setTextColor(TFT_WHITE);
		tft.setTextSize(1);
		strcpy(txt, "min:      max:");
		tft.setCursor(10, tpos + 5);
		tft.print(txt);
		dtostrf(tmin, 4, 1, txt);
		tft.setCursor(36, tpos);
		tft.print(txt);
		dtostrf(tmax, 4, 1, txt);
		tft.setCursor(96, tpos);
		tft.print(txt);
		if (modeflag & MANUAL)
			tft.setTextColor(TFT_LIGHTGREY);
		tpos += 10;
		dtostrf(min_drag, 4, 1, txt);
		tft.setCursor(36, tpos);
		tft.print(txt);
		dtostrf(max_drag, 4, 1, txt);
		tft.setCursor(96, tpos);
		tft.print(txt);
		tpos = boxWidth * xmax + (boxWidth >> 1) - 2;
		if(tpos > tft.width() - 2 * boxWidth)
			tpos = tft.width() - 2 * boxWidth;
		tft.setCursor(tpos , boxHeight * ymax + (boxHeight >> 1) - 2);
		tft.print("+");
		MINTEMP = min_drag;
		MAXTEMP = max_drag;
		if((MAXTEMP - MINTEMP) < 5)
		{
			MAXTEMP = MINTEMP + 5;
		}
	}
}
