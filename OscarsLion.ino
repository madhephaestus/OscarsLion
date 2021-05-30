#include <Arduino.h>
#include <lx16a-servo.h>
#include <WiiChuck.h>
#include <ESP32Servo.h>

LX16ABus servoBus;
LX16AServo servo(&servoBus, 1);
LX16AServo servo2(&servoBus, 2);
LX16AServo servo3(&servoBus, 3);
Accessory nunchuck1;
Servo mouth;

enum controlState {
	waitForCalibration, rcControl
};

controlState state = waitForCalibration;

void setup() {
	servoBus.beginOnePinMode(&Serial1, 15);
	Serial.begin(115200);
	servoBus.retry = 1; // enforce synchronous real time
	//servoBus.debug(true);
	Serial.println("Beginning Coordinated Servo Example");
	servo.disable();
	servo2.disable();
	servo3.disable();
	nunchuck1.begin();
	mouth.attach(13, 1000, 2000);
	mouth.write(0);

}
float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
	if (x > in_max)
		return out_max;
	if (x < in_min)
		return out_min;

	float divisor = (in_max - in_min);
	if (divisor == 0) {
		return -1; //AVR returns -1, SAM returns 0
	}
	return (x - in_min) * (out_max - out_min) / divisor + out_min;
}
void loop() {
	delay(30);
	nunchuck1.readData();    // Read inputs and update maps
	switch (state) {
	case waitForCalibration:{
		nunchuck1.printInputs(); // Print all inputs
		for (int i = 0; i < WII_VALUES_ARRAY_SIZE; i++) {
			Serial.println(
					"Controller Val " + String(i) + " = "
							+ String((uint8_t) nunchuck1.values[i]));
		}
		if(nunchuck1.values[15]>0){
			state=rcControl;
			Serial.println("servo.readLimits()");
			servo.calibrate(0, -9000, 9000);
			Serial.println("servo2.readLimits()");
			servo2.calibrate(0, -3000, 4500);
			Serial.println("servo3.readLimits()");
			servo3.calibrate(0, -5000, 4000);
			servo2.move_time_and_wait_for_sync(0, 0);
			servo3.move_time_and_wait_for_sync(0, 0);
			servo.move_time_and_wait_for_sync(0, 0);
		}
	}
		break;
	case rcControl: {
		if(nunchuck1.values[15]>0){
			state=waitForCalibration;
		}
		servo.pos_read();
		float Servo1Val = map(nunchuck1.values[1], 0, 255, -4500, 4500);
		int Servo2Val = map(nunchuck1.values[10] > 0 ? 0 : // Upper button pressed
				(nunchuck1.values[11] > 0 ? 255 : // Lower button pressed
						128) //neither pressed
				, 0, 255, 0, 180);
		float Servo3Val = fmap(nunchuck1.values[0], 0, 255, 9000, -9000); // z button
		float Servo4Val = fmap(nunchuck1.values[3], 0, 255, -4500, 4500); // z button

		mouth.write(Servo2Val);
		Serial.println(
				"Set " + String(Servo1Val) + " " + String(Servo2Val) + " "
						+ String(Servo3Val) + " "
								+ String(Servo4Val));
		servo.move_time_and_wait_for_sync(Servo3Val, 40);
		servo2.move_time_and_wait_for_sync(Servo1Val, 40);
		servo3.move_time_and_wait_for_sync(Servo4Val-Servo1Val, 40);
		servoBus.move_sync_start();
	}

		break;

	}

}
