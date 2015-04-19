#include <Adafruit_NeoPixel.h>

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

}

void loop() {
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.setPixelColor(1, strip.Color(0, 255, 0));
  strip.setPixelColor(2, strip.Color(0, 0, 255));
  strip.show();
  wake();
  
  squiggle(100, 4000, 2);
  delay(2000);
  
  //allForward(100);
  //delay(1000);
  //brakes();
  //allBackward(100);
  //delay(1000);
  //brakes();
  //sleep();
  //delay(1000);
}

void allForward(int forSpeed) {
  motorize( 0, forSpeed, 1);
  motorize( 1, forSpeed, 1);
  motorize( 2, forSpeed, 1);
  motorize( 3, forSpeed, 1);
}

void allBackward(int backSpeed) {
  motorize( 0, backSpeed, 0);
  motorize( 1, backSpeed, 0);
  motorize( 2, backSpeed, 0);
  motorize( 3, backSpeed, 0);
}

void brakes() {
  motorize( 0, 0, 0);
  motorize( 1, 0, 0);
  motorize( 2, 0, 0);
  motorize( 3, 0, 0);
}

void cwCircle(int circSpeed) {
  motorize(0, circSpeed, 1);
  motorize(1, circSpeed / 4, 1);
  motorize(2, circSpeed, 1);
  motorize(3, circSpeed / 4, 1);
  
}

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

void motorize(int motor, int speed, int direction) {
  
  // when direction is 0, default is backward
  boolean forward = LOW;
  boolean backward = HIGH;
  // otherwise direction is 1 and is set to forward
  if (direction == 1) {
    forward = HIGH;
    backward = LOW;
  }
  
  if (motor == 0) {
    digitalWrite(in1FL, forward);
    digitalWrite(in2FL, backward);
    analogWrite(speedFL, speed);
  }
  else if (motor == 1) {
    digitalWrite(in1FR, forward);
    digitalWrite(in2FR, backward);
    analogWrite(speedFR, speed);
  }
  else if (motor == 2) {
    digitalWrite(in1BL, forward);
    digitalWrite(in2BL, backward);
    analogWrite(speedBL, speed);
  }
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
