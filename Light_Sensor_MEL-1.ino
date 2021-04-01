/*
 Name:		Light_Sensor_MEL_1.ino
 Created:	12/2/2018 7:37:10 PM
 Author:	Danish Sajjad
*/

//Libraries Required
#include <DHT_U.h>
#include <DHT.h>
#include <wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT_U.h>
#include <DHT.h>
#include <math.h>
#include <EEPROM.h>

//Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT11 
#define LDRPin A0
#define sw 3
#define inA 2
#define inB 9
#define relay 6

//Custom Data Types
enum encoderRotation {anitClockwise, clockwise, stationary};
struct Stopwatch
{
	unsigned long int prev;
	unsigned long int curr;
	const unsigned int time = 3000;
};

volatile encoderRotation rotation;  //Enumerated data type stating the rotation of encoder
volatile byte aFlag = 0;            // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0;            // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte encoderP = 0;         //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte oldEncPos = 0;        //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte A = 0;                //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent
volatile byte B = 0;
volatile byte sw_s = LOW;          //Default state of encoder switch
volatile unsigned int threshold;    //Global variable for the stored threshold value
float light_Intensity;              //Global variable for light intensity
byte relayState = LOW;              //State of Relay
const int eepromAddr = 0;           //Address to store threshold value
bool reset = true;                  //Reset state is saved to make sure values are correctly written to eeprom
Stopwatch timer;                    //Timer for Threshold Screen
bool showThresh = false;            //Bool wheather to show threshold screen or not


Adafruit_SSD1306 display;              //Initialization of Display
DHT tempHumidSensor(DHTPIN, DHTTYPE);  //Initialization of Temperature sensor


//Interrupts
void pciSetup(byte pin)                
{
	*digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));  // enable pin
	PCIFR |= bit(digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
	PCICR |= bit(digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
void PinA() {
	cli(); //stop interrupts happening before we read pin values
	A = digitalRead(inA);
	B = digitalRead(inB);
	if (A == HIGH && B == HIGH && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
		encoderP++; //decrement the encoder's position count
		rotation = clockwise;
		bFlag = 0; //reset flags for the next turn
		aFlag = 0; //reset flags for the next turn
		adjustThreshHold();
	}
	else if (A == HIGH && B == LOW) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
	showEncoderPos();
	sei(); //restart interrupts
}
ISR(PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
	cli(); //stop interrupts happening before we read pin values
	//digitalWrite(13, digitalRead(8) and digitalRead(9));
	A = digitalRead(inA);
	B = digitalRead(inB);
	if (A == HIGH && B == HIGH && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
		encoderP--; //increment the encoder's position count
		rotation = anitClockwise;
		bFlag = 0; //reset flags for the next turn
		aFlag = 0; //reset flags for the next turn
		adjustThreshHold();
	}
	else if (A == LOW && B == HIGH) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
	showEncoderPos();
	sei(); //restart interrupts
}
void swState() {
	sw_s = !sw_s;
}

//Calculations
double voltage(int analog_Val) {
	return (double(analog_Val) / 1023.0)*5.00;
}
void readTempHumid(float &t, float &h) {
	h = tempHumidSensor.readHumidity();
	t = tempHumidSensor.readTemperature();
}
void calcIntensity() {
	float input_Voltage = voltage(analogRead(LDRPin));
	light_Intensity = lightIntensity(input_Voltage);
}
inline double lightIntensity(double v) {
	double exp = (2.16 - log10((5 / double(v)) - 1.0)) / (0.79);
	//Serial.println(exp);
	return pow(10.0, exp);
}

//Display Functions
void displayEnv() {
	float t, h;
	readTempHumid(t, h);
	display.setTextSize(1);
	display.setCursor(0, 10);
	display.setTextColor(WHITE);
	display.println("Temperature: " + String(t) + "C");
	display.println("Humidity:    " + String(h) + "%");

}
void displayLight() {
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.println("Intensity:");
	display.setTextSize(2);
	//tab(light_Intensity);
	display.println(String(light_Intensity, 0));
	display.setTextSize(1);
	display.println("LUX        TH: " + String(threshold));
	//return light_Intensity;
}
void displaySplash() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.setTextColor(WHITE);
	//display.setTextWrap(false);
	display.println("Light Intensity");
	display.println("Module");
	display.println("Submission By:");
	display.display();
	delay(2000);
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Danish Sajjad");
	display.println("Arafat Idrees");
	display.println("Arham Rahim");
	display.println("Danish Hassan");
	display.display();
	delay(2000);

}
void showRelayState(byte s) {
	//display.clearDisplay();
	//relayState = LOW;
	if (s == HIGH) {
		display.setCursor(107, 0);
		display.fillRoundRect(100, 0, 25, 8, 2, WHITE);
		display.setTextSize(1);
		display.setTextColor(BLACK);
		display.print("on");
	}
	else if (s == LOW) {
		display.setCursor(104, 0);
		display.drawRoundRect(100, 0, 25, 8, 2, WHITE);
		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.print("off");
	}
}
void displayThreshold() {
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.println("Threshold:");
	display.setTextSize(2);
	display.println(threshold);
	display.setTextSize(1);
	display.println("Current:  " + String(light_Intensity));
}
/*
void encoderPos() {
	cli();
	aState = HIGH; // Reads the "current" state of the outputA
	bLastState = digitalRead(inB);
	// If the outputB state is different to the outputA state, that means the encoder is rotating clockwise
	if (digitalRead(inB) != aState) {
		bState = digitalRead(inB);
		rotation = clockwise;
		while (aState != bState) {
		}
	}
	else {
		rotation = clockwise;
	}
	adjustThreshHold();
	Serial.print("Rotation: ");
	Serial.println(rotation);
	aLastState = aState; // Updates the previous state of the outputA with the current state
	sei();
}*/

//Misc Routeines
void toggleRelay(byte s) {
	digitalWrite(relay, s);
	showRelayState(s);
}
void adjustThreshHold() {
	if (rotation == clockwise && oldEncPos != encoderP) {
		threshold += 10;
		//rotation = stationary;
	}
	else if (rotation == anitClockwise && oldEncPos != encoderP) {
		threshold -= 10;
		//rotation = stationary;
	}
	if (reset == true) {
		EEPROM.put(eepromAddr, threshold);
		reset == false;
	}
	else {
		EEPROM.update(eepromAddr, threshold);
	}
	showThresh = true;
	timer.prev = millis();
	//Serial.println("Threshold: " + String(threshold));
}
void showEncoderPos() {
	if (oldEncPos != encoderP) {
		Serial.println(encoderP);
		oldEncPos = encoderP;
	}
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	pinMode(A0, INPUT);
	pinMode(sw, INPUT_PULLUP);
	pinMode(inA, INPUT_PULLUP);
	pinMode(inB, INPUT_PULLUP);
	pinMode(relay, OUTPUT);
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
		Serial.println(F("SSD1306 allocation failed"));
		for (;;); // Don't proceed, loop forever
	}
	displaySplash();
	//display.display();
	delay(500);
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	tempHumidSensor.begin();
	attachInterrupt(0, PinA, RISING); // set an interrupt on pin2, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (above)
	pciSetup(9);                      // set an interrupt on pin9, looking for a rising edge signal and executing the "ISR9" Interrupt Service Routine (above)
	attachInterrupt(digitalPinToInterrupt(sw), swState , FALLING); // set an interrupt for encoder switch to toggle display
	EEPROM.get(eepromAddr, threshold);
	Serial.println("Threshold: " + String(threshold));
	timer.prev = millis();
	timer.curr = millis();
}

// the loop function runs over and over again until power down or reset
void loop() {
	calcIntensity();
	if (sw_s == HIGH && showThresh == false){
		displayEnv();
	}
	else if (sw_s == LOW && showThresh == false) {
		displayLight();
	}
	else if (showThresh == HIGH) {
		timer.curr = millis();
		if (timer.curr - timer.prev <= timer.time) {
			displayThreshold();
		}
		else {
			showThresh = false;
		}
	}

	if (light_Intensity > threshold) {
		relayState = LOW;
	}
	else if (light_Intensity < threshold) {
		relayState = HIGH;
	}
	toggleRelay(relayState);

	display.display();         //Displays everything in display buffer
	display.clearDisplay();    //Clears display buffer for next iteration
}