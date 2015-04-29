#include <EDB.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h> // Adafruit NeoPixel Library

#include "RoverCommon.h"

ActionScore actionScore;

//hrm...with this size, we have room for 253 records
//doing that mod increases it to 338 records
#define TABLE_SIZE 1024// max size in ATMega328
#define MAX_RECORDS (TABLE_SIZE - sizeof(EDB_Header))/sizeof(ActionScore)

// The read and write handlers for using the EEPROM Library
void writer(unsigned long address, byte data)
{
	EEPROM.write(address, data);
}

byte reader(unsigned long address)
{
	return EEPROM.read(address);
}

// Create an EDB object with the appropriate write and read handlers
EDB db(&writer, &reader);

//use this format for DB entry cache:
//records[dbctx]= [ 0011 I 1101 I 1100 I 1001 ]
// for selecting randomly: records[dbctx] & (15 << random(4))


ActionScore epsilon_select(byte ctxpair, byte eps) {
	//eps indicates how often we should randomly select

	byte epsilon = (byte)random(100);
	int ctxinfo = 0;
        Serial.println("Receiving Context Pair");
	switch(ctxpair) {
		case (SAD << 4) | HAPPY:
			ctxinfo = (B00000011) << 8 | B10001100;
			break;
		case (FEARFUL << 4) | CALM:
			ctxinfo = (B01000010) << 8 | B10011101;
			break;
		case (DISTRACTED << 4) | FOCUSED:
			ctxinfo = (B00010110) << 8 | B10101110;
			break;
		case (MAD << 4) | CALM:
			ctxinfo = (B01010111) << 8 | B10111111;
			break;
	}

	if (epsilon < eps) {
                Serial.println("doing rndm action");
		ActionScore as;
		db.readRec(ctxinfo & (15 << (random(4) * 4)), EDB_REC as);
		return as;
	}
	else {
                Serial.println("doing best action");
		ActionScore actionScore1;
		ActionScore actionScore2;
		ActionScore actionScore3;
		ActionScore actionScore4;

		ActionScore maxScore;

		db.readRec(ctxinfo & ACTION0_MASK, EDB_REC actionScore1 );
		db.readRec(ctxinfo & ACTION1_MASK, EDB_REC actionScore2 );
		db.readRec(ctxinfo & ACTION2_MASK, EDB_REC actionScore3 );
		db.readRec(ctxinfo & ACTION3_MASK, EDB_REC actionScore4 );

		maxScore = actionScore1;
		if(maxScore.score > actionScore2.score) {
			maxScore = actionScore2;
		}
		if (maxScore.score > actionScore3.score) {
			maxScore = actionScore3;
		}
		if (maxScore.score > actionScore4.score) {
			maxScore = actionScore4;
		}

		return maxScore;
	/* for selecting w/max (we need four placeholders in mem?): 
	*  max(db.readRec(records[dbctx] & 15, EDB_REC actionScore).
	       db.readRec(records[dbctx] & 30, EDB_REC actionScore),
	       db.readRec(records[dbctx] & 60, EDB_REC actionScore),
	       db.readRec(records[dbctx] & 120, EDB_REC actionScore))
	*/
	}
}

byte reccodes2ctx() {
	//we should read in the serial from here
	//and determine the contexts we have and want to get to
	if (Serial.available() > 0) {
		char incomingByte = Serial.read();
		
		//now that we have the byte corresponding to the current ctx,
		//determine which ctx we want to trigger

		switch(incomingByte) {
			case 's':
				return (SAD << 4) | HAPPY;
			case 'f':
				return (FEARFUL << 4) | CALM;
			case 'd':
				return (DISTRACTED << 4) | FOCUSED;
			case 'a':
				return (MAD << 4) | CALM;
			case 'h':
				return (HAPPY << 4) | HAPPY;
			case 'o':
				return (FOCUSED << 4) | FOCUSED;
			case 'c':
				return (CALM << 4) | CALM;
			default:
				return (UNKNOWN << 4) | UNKNOWN;
		}
	}
	//example ctxes: happy, sad, angry, fearful, calm, focused, distracted
	return (UNKNOWN | (UNKNOWN << 4));
}

/*** Pin Layout ***/

// Motors //
int standby = 6;

int speedFL = 9; // front left speed
int in1FL = 4;   // front left forward
int in2FL = 2;   // front left backward

int speedFR = 10; // front right speed
int in1FR = 7;   // front right forward
int in2FR = 8;   // front right backward

int speedBL = 3; // back left speed
int in1BL = A1;   // back left forward
int in2BL = A0;   // back left backward

int speedBR = 5; // back right speed
int in1BR = A2;   // back right forward
int in2BR = A3;   // back right backward

// LEDs //
int ledData = 12;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, ledData, NEO_RGB + NEO_KHZ800);

void setup() {
	/*** Pin Setup ***/
	pinMode(standby, OUTPUT);

	pinMode(speedFL, OUTPUT);
	pinMode(in1FL, OUTPUT);
	pinMode(in2FL, OUTPUT);
	
	pinMode(speedFR, OUTPUT);
	pinMode(in1FR, OUTPUT);
	pinMode(in2FR, OUTPUT);
 
	pinMode(speedBL, OUTPUT);
	pinMode(in1BL, OUTPUT);
	pinMode(in2BL, OUTPUT);
	
	pinMode(speedBR, OUTPUT);
	pinMode(in1BR, OUTPUT);
	pinMode(in2BR, OUTPUT);

	/** LED Setup **/
	strip.begin();
	strip.show();

	/** Database Setup **/
	Serial.begin(9600);
	Serial.print("Max DB records: ");
	Serial.println(MAX_RECORDS);

	//check if the DB exists
	db.open(0);

	//if record count is 0 (I think?), populate stub DB
	if (db.count() == 0) {
		Serial.println("Creating DB");
		db.create(0, TABLE_SIZE, sizeof(actionScore));


		for (int recno = 0; recno < 16; recno++) {
			actionScore.score = 128; // set it in the middle for +- adjustment
			actionScore.id = (byte) recno;
			actionScore.action = (byte)recno;
			switch (recno) {
				case B0000:
				case B0011:
				case B1000:
				case B1100:
					actionScore.ctxpair = (SAD << 4) | HAPPY;
					break;
				case B0100:
				case B0010:
				case B1001:
				case B1101:
					actionScore.ctxpair = (FEARFUL << 4) | CALM;
					break;
				case B0001:
				case B0110:
				case B1010:
				case B1110:
					actionScore.ctxpair = (DISTRACTED << 4) | FOCUSED;
					break;
				case B0101:
				case B0111:
				case B1011:
				case B1111:
					actionScore.ctxpair = (MAD << 4) | CALM;
					break;
			}
			db.appendRec(EDB_REC actionScore);
		}

	}
	else {
		Serial.println("Using existing DB");
	}

	//wait for the ready message from the mini computer?
}

void loop() {
        Serial.println("Retrieving mood");
  	//insert retrieving EEG return codes here
	//
	byte ctxpair = reccodes2ctx();
        Serial.print((ctxpair & FROM_CTX_MASK) >> 4);
        Serial.print(" ");
        Serial.println(ctxpair & TO_CTX_MASK);
        

	switch((ctxpair & FROM_CTX_MASK) >> 4) {
		case SAD:
			strip.setPixelColor(0, SAD_COLOR);
			break;
		case MAD:
			strip.setPixelColor(0, MAD_COLOR);
			break;
		case FEARFUL:
			strip.setPixelColor(0, FEARFUL_COLOR);
			break;
		case DISTRACTED:
			strip.setPixelColor(0, DISTRACTED_COLOR);
			break;
		case HAPPY:
			strip.setPixelColor(0, HAPPY_COLOR);
			break;
		case CALM:
			strip.setPixelColor(0, CALM_COLOR);
			break;
		case FOCUSED:
			strip.setPixelColor(0, FOCUSED_COLOR);
			break;
		default:
			strip.setPixelColor(0, UNKNOWN_COLOR);
			break;
	}

	switch(ctxpair & TO_CTX_MASK) {
		case SAD:
			strip.setPixelColor(1, SAD_COLOR);
			break;
		case MAD:
			strip.setPixelColor(1, MAD_COLOR);
			break;
		case FEARFUL:
			strip.setPixelColor(1, FEARFUL_COLOR);
			break;
		case DISTRACTED:
			strip.setPixelColor(1, DISTRACTED_COLOR);
			break;
		case HAPPY:
			strip.setPixelColor(1, HAPPY_COLOR);
			break;
		case CALM:
			strip.setPixelColor(1, CALM_COLOR);
			break;
		case FOCUSED:
			strip.setPixelColor(1, FOCUSED_COLOR);
			break;
		default:
			strip.setPixelColor(1, UNKNOWN_COLOR);
			break;
	}
	strip.show();

	if (((ctxpair & FROM_CTX_MASK) >> 4) == (ctxpair & TO_CTX_MASK)) {
		//don't do anything, not even the epsilon select,
		//if the desired context and current context are the same
                Serial.println("Same current and desired moods, doing nothing");
		delay(2000);
		return;
	}
        Serial.print("Perform action ");
	actionScore = epsilon_select(ctxpair, 10);
        Serial.println(actionScore.action);
	switch(actionScore.action) {
		case 0: // Drive forward, turn around, drive back
			wake();
			allForward(100);
			delay(2000);
			brakes();
			leftInPlace(180);
			allForward(100);
			delay(2000);
			sleep();
			break;
		case 1: // Drive backward
			wake();
			allBackward(100);
			delay(2000);
			brakes();
			sleep();
			break;
		case 2: // Drive in a clockwise circle
			wake();
			cwCircle(100);
			delay(8000);
			sleep();
			break;
		case 3: // Drive in a counter-clockwise circle
			wake();
			ccwCircle(100);
			delay(8000);
			sleep();
			break;
		case 4: // Drive in a slight squiggle for 5 seconds
			wake();
			squiggle(100, 5, 2);
			sleep();
			break;
		case 5: // Drive in a larger squiggle for 5 seconds
			wake();
			squiggle(100, 3, 4);
			sleep();
			break;
		case 6: // Traverse a square
			wake();
			allForward(100);
			delay(2000);
			leftInPlace(90);
			allForward(100);
			delay(2000);
			leftInPlace(90);
			allForward(100);
			delay(2000);
			leftInPlace(90);
			allForward(100);
			delay(2000);
			sleep();
			break;
		case 7: // Traverse a triangle
			wake();
			allForward(100);
			delay(2000);
			leftInPlace(60);
			allForward(100);
			delay(2000);
			leftInPlace(60);
			allForward(100);
			delay(2000);
			sleep();
			break;
		case 8: // Spin in place
			wake();
			leftInPlace(360);
			rightInPlace(360);
			sleep();
			break;
		case 9: // Short back and forth movements
			wake();
			allForward(100);
			delay(100);
			allBackward(100);
			delay(100);
			allForward(100);
			delay(100);
			allBackward(100);
			delay(100);
			allForward(100);
			delay(100);
			allBackward(100);
			delay(100);
			allForward(100);
			delay(100);
			allBackward(100);
			delay(100);
			sleep();
			break;
		case 10: // Half circle, then come back NEEDS TWEAKING
			wake();
			cwCircle(100);
			delay(4000);
			rightInPlace(90);
			allForward(100);
			delay(2000); 
			sleep();
			break;
		case 11: // Traverse a clover NEEDS TWEAKING
			wake();
			cwCircle(80);
			delay(2000);
			leftInPlace(90);
			cwCircle(80);
			delay(2000);
			leftInPlace(90);
			cwCircle(80);
			delay(2000);
			leftInPlace(90);
			cwCircle(80);
			delay(2000);
			sleep();
			break;
		case 12: // Turn left and drive
			wake();
			leftInPlace(90);
			allForward(100);
			delay(2000);
			sleep();
			break;
		case 13: // Turn right and drive
			wake();
			rightInPlace(90);
			allForward(100);
			delay(2000);
			sleep();
			break;
		case 14: // Wiggle in place
			wake();
			leftInPlace(30);
			rightInPlace(30);
			leftInPlace(30);
			rightInPlace(30);
			leftInPlace(30);
			rightInPlace(30);
			leftInPlace(30);
			rightInPlace(30);
			leftInPlace(30);
			rightInPlace(30);
			leftInPlace(30);
			rightInPlace(30);
			sleep();
			break;
		case 15: // Traverse a spiral NEEDS TWEAKING
			wake();
			ccwCircle(120);
			delay(3000);
			ccwCircle(100);
			delay(2800);
			ccwCircle(80);
			delay(2600);
			ccwCircle(60);
			delay(2400);
			ccwCircle(40);
			delay(2200);
			sleep();
			break;
		default: // Do nothing
			break;
	}
        Serial.println("Checking result mood");
	byte newctxpair = reccodes2ctx();
	if ((newctxpair & FROM_CTX_MASK) >> 4 == ctxpair & TO_CTX_MASK) {
                Serial.println("Changed to desired context successfully");
		actionScore.score++;
	} else {
                Serial.println("Didn't change to desired context successfully");
		actionScore.score--;
	}
	db.updateRec(actionScore.id, EDB_REC actionScore);
	//
}

/*** Motor Functions ***/
// Moves the rover forward at given speed
void allForward(int forSpeed) {
	motorize( 0, forSpeed, 1);
	motorize( 1, forSpeed, 1);
	motorize( 2, forSpeed, 1);
	motorize( 3, forSpeed, 1);
}

// Moves the rover backward at given speed
void allBackward(int backSpeed) {
	motorize( 0, backSpeed, 0);
	motorize( 1, backSpeed, 0);
	motorize( 2, backSpeed, 0);
	motorize( 3, backSpeed, 0);
}

// Moves the rover in a clockwise circle at given speed
void cwCircle(int circSpeed) {
	motorize(0, circSpeed, 1);
	motorize(1, circSpeed / 4, 1);
	motorize(2, circSpeed, 1);
	motorize(3, circSpeed / 4, 1);
}

// Moves the rover in a counter-clockwise circle at given speed
void ccwCircle(int circSpeed) {
	motorize(0, circSpeed / 4, 1);
	motorize(1, circSpeed, 1);
	motorize(2, circSpeed / 4, 1);
	motorize(3, circSpeed, 1);
}

// Moves the rover forward in a squiggly line for a given length in seconds
// Larger squigFactors result in more squiggly lines, must be larger than 1
void squiggle(int squigSpeed, int squigTime, int squigFactor) {
	for (int i = 0; i < squigTime; i += 2000) {
		motorize(0, squigSpeed, 1);
		motorize(1, squigSpeed / squigFactor, 1);
		motorize(2, squigSpeed, 1);
		motorize(3, squigSpeed / squigFactor, 1);
		delay(1000);
		motorize(0, squigSpeed / squigFactor, 1);
		motorize(1, squigSpeed, 1);
		motorize(2, squigSpeed / squigFactor, 1);
		motorize(3, squigSpeed, 1);
		delay(1000);
	}
	brakes();
}

// Turn the rover left in place the given number of degrees
void leftInPlace(double degrees) {
	motorize( 0, 80, 1);
	motorize( 1, 80, 0);
	motorize( 2, 80, 1);
	motorize( 3, 80, 0);
	delay(6000 * (degrees / 360)); // UNTESTED
	brakes();
}

// Turn the rover right in place the given number of degrees
void rightInPlace(double degrees) {
	motorize( 0, 80, 0);
	motorize( 1, 80, 1);
	motorize( 2, 80, 0);
	motorize( 3, 80, 1);
	delay(6000 * (degrees / 360)); // UNTESTED
	brakes();
}

// Send speeds of zero in backward direction to stop wheels
void brakes() {
	motorize( 0, 0, 0);
	motorize( 1, 0, 0);
	motorize( 2, 0, 0);
	motorize( 3, 0, 0);
}

// Motor instructions for speed and direction
void motorize(int motor, int speed, int direction) {
	// when direction is 0, default is backward
	boolean forward = LOW;
	boolean backward = HIGH;
	// otherwise direction is 1 and is set to forward
	if (direction == 1) {
		forward = HIGH;
		backward = LOW;
	}
	// Write to pins:
	// Front left
	if (motor == 0) {
		digitalWrite(in1FL, forward);
		digitalWrite(in2FL, backward);
		analogWrite(speedFL, speed);
	}
	// Front Right
	else if (motor == 1) {
		digitalWrite(in1FR, forward);
		digitalWrite(in2FR, backward);
		analogWrite(speedFR, speed);
	}
	// Back Left
	else if (motor == 2) {
		digitalWrite(in1BL, forward);
		digitalWrite(in2BL, backward);
		analogWrite(speedBL, speed);
	}
	// Back Right
	else {
		digitalWrite(in1BR, forward);
		digitalWrite(in2BR, backward);
		analogWrite(speedBR, speed);
	}
}

void sleep() {
	digitalWrite(standby, LOW); // enable standby mode
}

void wake() {
	digitalWrite(standby, HIGH); // disable standby mode
}

