#include <Arduino.h>
#include <U8g2lib.h>
#include <SoftWire.h>
#include <AS3935.h>

#ifdef u8g2_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef u8g2_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ A5, /* data=*/ A4, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C
AS3935 as3935;

static const unsigned char storm_icon_bits[] PROGMEM = {
   0xe0, 0x07, 0xf0, 0x0f, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xe0, 0x07, 0xc0, 0x00, 0xc0, 0x01,
   0x80, 0x01, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x00 };

void int2Handler(void)
{
	as3935.interruptHandler();
}

void readRegs(uint8_t start, uint8_t end)
{
	for (uint8_t reg = start; reg < end; ++reg) {
		delay(50);
		uint8_t val;
		as3935.readRegister(reg, val);

		Serial.print("Reg: 0x");
		Serial.print(reg, HEX);
		Serial.print(": 0x");
		Serial.println(val, HEX);
		Serial.flush();
	}
	Serial.print("State: ");
	Serial.println(as3935.getState(), DEC);

	Serial.println("-------------");
}

void printInterruptReason(Stream &s, uint8_t value, const char *prefix = nullptr)
{
	if (value & AS3935::intNoiseLevelTooHigh) {
		if (prefix)
			s.print(prefix);
		s.println(F("Noise level too high"));
	}
	if (value & AS3935::intDisturberDetected) {
		if (prefix)
			s.print(prefix);
		s.println(F("Disturber detected"));
	}
	if (value & AS3935::intLightningDetected) {
		if (prefix)
			s.print(prefix);
		s.println(F("Lightning detected"));
	}
}

// void pre(void)
// {
//   u8g2.clear();
//   u8g2.setFont(u8g2_font_6x10_tf);
//   u8g2.drawXBMP(0, 0, 16, 16, storm_icon_bits);
//   u8g2.setFont(u8g2_font_unifont_t_symbols);
//  //u8g2.inverse();
//   //u8g2.print(" u8g2 Library ");
//   //u8g2.setFont(u8g2_font_chroma48medium8_r);  
//   //u8g2.noInverse();
//   u8g2.setCursor(0,1);
// }

void u8g2_prepare(void) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void u8g2_extra_page(uint8_t a)
{
  u8g2.drawStr( 0, 0, "Unicode");
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.setFontPosTop();
  u8g2.drawUTF8(0, 24, "☀ ☁");
  switch(a) {
    case 0:
    case 1:
    case 2:
    case 3:
      u8g2.drawUTF8(a*3, 36, "☂");
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      u8g2.drawUTF8(a*3, 36, "☔");
      break;
  }
}

void setup() {
	Serial.begin(9600);
	as3935.initialise(A4, A5, 0x03, 3, true, NULL);
	// as3935.calibrate();
	as3935.start();
	readRegs(0, 0x09);
	as3935.setNoiseFloor(0);
	attachInterrupt(2, int2Handler, RISING);
	u8g2.begin();
}

void loop() {
	u8g2_prepare();
	u8g2.drawStr(30, 32, "detecting...");
	u8g2.setBitmapMode(false /* solid */);	
 	u8g2.sendBuffer();
	delay(1000);
	u8g2.clearBuffer();
	u8g2.drawStr(40, 0, "LIGHTNING");
	u8g2.setBitmapMode(false /* solid */);
	u8g2.drawXBMP(0, 0, 16, 16, storm_icon_bits);
	u8g2.drawXBMP(128-16, 0, 16, 16, storm_icon_bits);
	u8g2.drawStr(0, 8*3, "Distance:  23 km");
	u8g2.drawStr(0, 8*4, "Energy:    7192");
	u8g2.sendBuffer();
	delay(5000);
	if (as3935.process()) 
	{
		uint8_t flags = as3935.getInterruptFlags();
		uint8_t dist = as3935.getDistance();

		Serial.println("-------------------");
		Serial.println("Interrupt!");
		Serial.println("Reason(s):");
		printInterruptReason(Serial, flags, "    ");
		if (AS3935::intLightningDetected & flags) {
			Serial.print("Distance: ");
			Serial.println(dist, DEC);
		}
  	}

	if (as3935.getBusError()) 
	{
		Serial.println("Bus error!");
		as3935.clearBusError();
	}
}