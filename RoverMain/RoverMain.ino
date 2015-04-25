#include <EDB.h>
#include <EEPROM.h>

#define TABLE_SIZE 1024// max size in ATMega328

#include <Adafruit_NeoPixel.h> // Adafruit NeoPixel Library

typedef struct {
	byte action;
	byte ctxpair; // maybe we can use the first four bits for the prev and last four for next
	byte score;
	byte id;
} ActionScore;

ActionScore actionScore;

//hrm...with this size, we have room for 253 records
//doing that mod increases it to 338 records

#define MAX_RECORDS (TABLE_SIZE - sizeof(EDB_Header))/sizeof(ActionScore)

#define FROM_CTX_MASK B11110000
#define TO_CTX_MASK   B00001111

#define SAD B0000
#define MAD B0001
#define FEARFUL B0010
#define DISTRACTED B0011
#define HAPPY B1111
#define CALM B1110
#define FOCUSED B1100
#define UNKNOWN B0100

// limit ourselves to possibly four actions for a given context pair
#define ACTION0_MASK   15    //B1111000000000000
#define ACTION1_MASK   240   //B0000111100000000
#define ACTION2_MASK   3840  //B0000000011110000
#define ACTION3_MASK   61440 //B0000000000001111

//int lookups[256];
//lookups[0b00101010] = 0b0011110111001001;
//int lookups[256]; //we should probably burn this into prog memory 

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
		ActionScore as;
		db.readRec(ctxinfo & (15 << (random(4) * 4)), EDB_REC as);
		return as;
	}
	else {
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
	//insert retrieving EEG return codes here
	//
	byte ctxpair = reccodes2ctx();

	switch((ctxpair & FROM_CTX_MASK) >> 4) {
		case SAD:
			strip.setPixelColor(0, strip.Color(0, 0 , 255));
			break;
		case MAD:
			strip.setPixelColor(0, strip.Color(255, 0 , 0));
			break;
		case FEARFUL:
			strip.setPixelColor(0, strip.Color(255, 0 , 140));
			break;
		case DISTRACTED:
			strip.setPixelColor(0, strip.Color(165, 0 , 255));
			break;
		case HAPPY:
			strip.setPixelColor(0, strip.Color(0, 255, 65));
			break;
		case CALM:
			strip.setPixelColor(0, strip.Color(0, 248 , 255));
			break;
		case FOCUSED:
			strip.setPixelColor(0, strip.Color(255, 255, 213));
			break;
		default:
			strip.setPixelColor(0, strip.Color(255, 123 , 0));
			break;
	}

	switch(ctxpair & TO_CTX_MASK) {
		case SAD:
			strip.setPixelColor(1, strip.Color(0, 0 , 255));
			break; 
		case MAD:
			strip.setPixelColor(1, strip.Color(255, 0 , 0));
			break;
		case FEARFUL:
			strip.setPixelColor(1, strip.Color(255, 0 , 140));
			break;
		case DISTRACTED:
			strip.setPixelColor(1, strip.Color(165, 0 , 255));
			break;
		case HAPPY:
			strip.setPixelColor(1, strip.Color(0, 255, 65));
			break;
		case CALM:
			strip.setPixelColor(1, strip.Color(0, 248, 255));
			break;
		case FOCUSED:
			strip.setPixelColor(1, strip.Color(255, 255 , 213));
			break;
		default:
			strip.setPixelColor(1, strip.Color(255, 123 , 0));
			break;
	}
	strip.show();

	if (((ctxpair & FROM_CTX_MASK) >> 4) == (ctxpair & TO_CTX_MASK)) {
		//don't do anything, not even the epsilon select,
		//if the desired context and current context are the same
		delay(2000);
		return;
	}

	actionScore = epsilon_select(ctxpair, 10);
	switch(actionScore.action) {
		case 0: // Drive forward, turn around, drive back UNTESTED
			wake();
			allForward(100);
			delay(1000);
			brakes();
			leftInPlace(180);
			allForward(100);
			delay(1000);
			sleep();
			break;
		case 1: // Drive backward UNTESTED
			wake();
			allBackward(50);
			delay(2000);
			brakes();
			sleep();
			break;
		case 2: // Drive in a clockwise circle UNTESTED
			wake();
			cwCircle(50);
			delay(2000); // PROBABLY NEED TO CHANGE DELAY
			sleep();
			break;
		case 3: // Drive in a counter-clockwise circle UNTESTED
			wake();
			ccwCircle(50);
			delay(2000); // PROBABLY NEED TO CHANGE DELAY
			sleep();
			break;
		case 4: // Drive in a slight squiggle for 3 seconds UNTESTED
			wake();
			squiggle(75, 3, 2);
			sleep();
			break;
		case 5: // Drive in a larger squiggle for 3 seconds UNTESTED
			wake();
			squiggle(75, 3, 4);
			sleep();
			break;
		case 6: // Traverse a square UNTESTED
			wake();
			allForward(50);
			delay(1000);
			leftInPlace(90);
			allForward(50);
			delay(1000);
			leftInPlace(90);
			allForward(50);
			delay(1000);
			leftInPlace(90);
			allForward(50);
			delay(1000);
			sleep();
			break;
		case 7: // Traverse a triangle UNTESTED
			wake();
			allForward(50);
			delay(1000);
			leftInPlace(60);
			allForward(50);
			delay(1000);
			leftInPlace(60);
			allForward(50);
			delay(1000);
			sleep();
			break;
		case 8: // Spin in place UNTESTED
			wake();
			leftInPlace(360);
			rightInPlace(360);
			sleep();
			break;
		case 9: // Short back and forth movements UNTESTED
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
		case 10: // Half circle, then come back UNTESTED
			wake();
			cwCircle(50);
			delay(1000); // PROBABLY NEED TO CHANGE DELAY
			rightInPlace(90);
			allForward(50);
			delay(800);  // PROBABLY NEED TO CHANGE DELAY
			sleep();
			break;
		case 11: // Traverse a clover UNTESTED
			wake();
			cwCircle(50);
			delay(700); // PROBABLY NEED TO CHANGE DELAY
			leftInPlace(90);
			cwCircle(50);
			delay(700); // PROBABLY NEED TO CHANGE DELAY
			leftInPlace(90);
			cwCircle(50);
			delay(700); // PROBABLY NEED TO CHANGE DELAY
			leftInPlace(90);
			cwCircle(50);
			delay(700); // PROBABLY NEED TO CHANGE DELAY
			sleep();
			break;
		case 12: // Turn left and drive UNTESTED
			wake();
			// ACTION
			sleep();
			break;
		case 13: // Turn right and drive UNTESTED
			wake();
			// ACTION
			sleep();
			break;
		case 14: // Small spins back and forth UNTESTED
			wake();
			// ACTION
			sleep();
			break;
		case 15: // Traverse a spiral UNTESTED
			wake();
			// ACTION
			sleep();
			break;
		default: // Do nothing
			break;
	}
	byte newctxpair = reccodes2ctx();
	if ((newctxpair & FROM_CTX_MASK) >> 4 == ctxpair & TO_CTX_MASK) {
		actionScore.score++;
	} else {
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
void leftInPlace(int degrees) {
	motorize( 0, 50, 1);
	motorize( 1, 50, 0);
	motorize( 2, 50, 1);
	motorize( 3, 50, 0);
	delay(1000 * (degrees / 360)); // UNTESTED
	brakes();
}

// Turn the rover right in place the given number of degrees
void rightInPlace(int degrees) {
	motorize( 0, 50, 0);
	motorize( 1, 50, 1);
	motorize( 2, 50, 0);
	motorize( 3, 50, 1);
	delay(1000 * (degrees / 360)); // UNTESTED
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

