#include <EDB.h>
#include <EEPROM.h>
#define TABLE_SIZE 1024// max size in ATMega328

typedef struct {
  byte action;
  byte prev_ctx; // maybe we can use the first four bits for the prev and last four for next
  //byte desired_ctx;
  byte score;
  byte id;
} ActionScore;

ActionScore actionScore;

//hrm...with this size, we have room for 253 records
//doing that mod increases it to 338 records

#define MAX_RECORDS (TABLE_SIZE - sizeof(EDB_Header))/sizeof(ActionScore)

#define FROM_CTX_MASK 0b11110000
#define TO_CTX_MASK   0b00001111

// limit ourselves to possibly four actions for a given context pair
#define A0_MASK   0b1111000000000000
#define A1_MASK   0b0000111100000000
#define A2_MASK   0b0000000011110000
#define A3_MASK   0b0000000000001111

//int lookups[256];
//lookups[0b00101010] = 0b0011110111001001;
int lookups[256];

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

	if (epsilon < eps) {
		ActionScore as;
		db.readRec(lookups[ctxpair] & (15 << (random(4) * 4)), EDB_REC as)
		return as;
	}
	else {
		ActionScore actionScore1;
		ActionScore actionScore2;
		ActionScore actionScore3;
		ActionScore actionScore4;

		ActionScore maxScore;

		db.readRec(records[ctxpair] & (15 << 0), EDB_REC actionScore1 );
		db.readRec(records[ctxpair] & (15 << 1), EDB_REC actionScore2 );
		db.readRec(records[ctxpair] & (15 << 2), EDB_REC actionScore3 );
		db.readRec(records[ctxpair] & (15 << 3), EDB_REC actionScore4 );

		maxScore = actionScore1;
		if(maxScore > actionScore2.score) {
			maxScore = actionScore2;
		}
		if (maxScore > actionScore3.score) {
			maxScore = actionScore3;
		}
		if (maxScore > actionScore4.score) {
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

	//example ctxes: happy, sad, angry, fearful, calm, focused, distracted
	return 0;
}

void setup() {
	Serial.begin(9600);
	Serial.print("Max DB records: ");
	Serial.println(MAX_RECORDS);

	//check if the DB exists
	db.open(0);

	//if record count is 0 (I think?), populate stub DB
	if (db.count() == 0) {
		Serial.println("Creating DB");
		db.create(0, TABLE_SIZE, sizeof(actionScore));

		/*
		for (int recno = 1; recno < (MAX_CTXS*MAX_CTXS); recno++) {
			actionScore.score = 128; // set it in the middle for +- adjustment
			actionScore.id = (byte) recno;
			actionScore.ctxpair = (fromctx << 4) | (toctx);
			db.appendRec(EDB_REC actionScore);
		}
		*/
	}
	else {
		Serial.println("Using existing DB");
	}

	//wait for the ready message from the mini computer?
}

void loop() {
	//insert retrieving EEG return codes here
	/*
	byte ctxpair = reccodes2ctx();
	actionScore = epsilon_select(ctxpair, 10);
	switch(actionScore.action) {
		case 0:
			//some action
			break;
		default:
			break;
	}
	byte newctxpair = reccodes2ctx();
	if ((newctxpair & FROM_CTX_MASK) >> 4 == ctxpair & TO_CTX_MASK) {
		actionScore.score++;
	} else {
		actionScore.score--;
	}
	db.updateRec(actionScore.id, EDB_REC actionScore);
	*/
}
