typedef struct {
	byte action;
	byte ctxpair; // maybe we can use the first four bits for the prev and last four for next
	byte score;
	byte id;
} ActionScore;

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

#define SAD_COLOR           strip.Color(  0,   0, 255)
#define MAD_COLOR           strip.Color(255,   0,   0)
#define FEARFUL_COLOR       strip.Color(255,   0, 140)
#define DISTRACTED_COLOR    strip.Color(165,   0, 255)
#define HAPPY_COLOR         strip.Color(  0, 255,  65)
#define CALM_COLOR          strip.Color(  0, 248, 255)
#define FOCUSED_COLOR       strip.Color(255, 255, 213)
#define UNKNOWN_COLOR       strip.Color(255, 123,   0)
