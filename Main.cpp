/*
 *	Main.cpp
 *
 *
 *      Author: Juande Estrella
 *
 *  Importante probrar primero sin helices.
 * Cuando estemos seguro, ir poniendo las helices que vamos a calibrar.
 *
 *
 *
*/

#include "Motor/Motor.h"
#include "Battery/Battery.h"
#include "ControlMode/ControlMode.h"
#include "Camera/ControlPixy.h"

Battery bateria(pinBattery);

Bluetooth bluetooth(rxBluetooth,txBluetooth);

Motor motor1(pinM1);
Motor motor2(pinM2);
Motor motor3(pinM3);
Motor motor4(pinM4);

Leds leds(ledWhite, ledBlue, ledRedFront, ledGreenFront, ledRedBack);

MPU6050 mpu6050(0x68, sdaMPU, sclMPU, numberSampleMPU);

ControlPixy pixy;

ControlMode controlMode;

void setup() {
	Serial.begin(baudrates);

	bluetooth.initialize();

	leds.initialize();

	leds.offAllLeds();
	leds.onLedBlue();

	if(!mpu6050.initialize() || bateria.isLowBattery()){
		leds.offAllLeds();
		leds.onLedWarning();
		Serial.println("ERROR no se inicia MPU o la Bater�a esta baja");
		while (true)delay(10);
	}

	pixy.initialize();

	mpu6050.calculateOffset(controlMode.isCalculateGyroscope(),controlMode.isCalculateAcelerometer());

	motor1.initialize();
	motor2.initialize();
	motor3.initialize();
	motor4.initialize();

	Serial.print("Conectarse al dron y bajar throttle al minimo");
	while (bluetooth.getThrottle() != pulseMinMotor){
	  bluetooth.updatePulse();
	}

	Serial.println("OK");
	leds.offAllLeds();
	mpu6050.updateLoopTimer();
	mpu6050.updateExecutionTimer(mpu6050.getLoopTimer());
}


void loop() {
	controlMode.onLedAccordingMode(leds);

	while (micros() - mpu6050.getLoopTimer() < usCiclo);
	mpu6050.updateExecutionTimer(mpu6050.getLoopTimer());
	mpu6050.updateLoopTimer();

	mpu6050.readSensors();
	if(controlMode.isCalculateAcelerometer()){
		mpu6050.processAccelerometer();
	}

	if(controlMode.isCalculateGyroscope()) {
			mpu6050.calculateAngle();
	}

	if(!controlMode.isModeAcrobatic()){
		controlMode.PIDAnglee(bluetooth, mpu6050);
	}
	controlMode.PIDSpeed(bluetooth, mpu6050);

	motor1.controller(bluetooth.getThrottle(), controlMode.getPidPitch(),-controlMode.getPidRoll(), -controlMode.getPidYaw());
	motor2.controller(bluetooth.getThrottle(), controlMode.getPidPitch(), controlMode.getPidRoll(), controlMode.getPidYaw());
	motor3.controller(bluetooth.getThrottle(), -controlMode.getPidPitch(), controlMode.getPidRoll(), -controlMode.getPidYaw());
	motor4.controller(bluetooth.getThrottle(), -controlMode.getPidPitch(),-controlMode.getPidRoll(), controlMode.getPidYaw());

	if(!controlMode.isModeUp() && !controlMode.isModeDown() && !controlMode.isModeAutomatic()){
		bluetooth.updatePulse();
	}

	if(controlMode.isModeUp()){
		int throttle = bluetooth.getThrottle() + incrementThrottle;
		if(throttle <= degreeMaxUp){
			bluetooth.modifyThrottle(throttle);
		}else{
			leds.offAllLeds();
			controlMode.activateModeStable();
		}
	}

	if(controlMode.isModeDown() || bateria.isLowBattery()){
			int throttle = bluetooth.getThrottle() - incrementThrottle;
			if(throttle >= degreeMin){
				bluetooth.modifyThrottle(throttle);
			}else{
				leds.offAllLeds();
				controlMode.activateModeStable();
			}
	}

	if(controlMode.isModeAutomatic()){
		pixy.updateBlocks();
		pixy.updatePulse(bluetooth);
	}
}


int main(){

	init();

	setup();

	while(true){
		loop();
	}

	return 0;
}
