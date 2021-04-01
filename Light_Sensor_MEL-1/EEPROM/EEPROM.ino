/*
 Name:		EEPROM.ino
 Created:	12/11/2018 12:41:20 AM
 Author:	Danish Sajjad
*/
#include <EEPROM.h>

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	for (int i = 0; i < EEPROM.length(); i++) {
		EEPROM.write(i, 0);
	}

	// turn the LED on when we're done
	digitalWrite(13, HIGH);
	/*
	double test = 1.25;
	EEPROM.put(0, test);
	Serial.println("Data Put Successfully");
	EEPROM.get(0, test);
	Serial.println(test);
	EEPROM.update(0, 1.5);
	Serial.println("Data Updated");
	EEPROM.get(0, test);
	Serial.println(test);*/

}

// the loop function runs over and over again until power down or reset
void loop() {
  
}
