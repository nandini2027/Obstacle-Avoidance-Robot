#include <AFMotor.h>
#include <Servo.h>
#include <SoftwareSerial.h>

//BLUETOOTH SERIAL
SoftwareSerial BT(A2, A3);  // RX=A2, TX=A3

//Ultrasonic Pins 
#define TRIG A0
#define ECHO A1

//Servo
Servo scanServo;   // D10

//Motors
AF_DCMotor leftMotor(1);
AF_DCMotor rightMotor(2);

//Modes
#define AUTONOMOUS 1
#define MANUAL     2

int currentMode = AUTONOMOUS;

//Manual control timing 
char manualCmd = 'S';
unsigned long manualCmdEndTime = 0;

const unsigned long MANUAL_MOVE_TIME = 300;  // 0.3 seconds
const unsigned long TURN_TIME = 350;         // approx 60 degrees turn

// Speeds 
int forwardSpeed = 180;
int turnSpeed    = 160;
int reverseSpeed = 160;

// Distance 
int detectDistance = 20;
int tightSpace     = 15;


//  ULTRASONIC FUNCTIONS

long getRawDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 25000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

long getDistance() {
  long d = getRawDistance();
  if (d < 0 || d > 150 || d < 5) return 200;
  return d;
}


// MOTOR FUNCTIONS

void forward() {
  leftMotor.setSpeed(forwardSpeed);
  rightMotor.setSpeed(forwardSpeed);
  leftMotor.run(FORWARD);
  rightMotor.run(FORWARD);
}

void backward() {
  leftMotor.setSpeed(reverseSpeed);
  rightMotor.setSpeed(reverseSpeed);
  leftMotor.run(BACKWARD);
  rightMotor.run(BACKWARD);
}

void turnLeft() {
  leftMotor.setSpeed(turnSpeed);
  rightMotor.setSpeed(turnSpeed);
  leftMotor.run(BACKWARD);
  rightMotor.run(FORWARD);
}

void turnRight() {
  leftMotor.setSpeed(turnSpeed);
  rightMotor.setSpeed(turnSpeed);
  leftMotor.run(FORWARD);
  rightMotor.run(BACKWARD);
}

void stopRobot() {
  leftMotor.run(RELEASE);
  rightMotor.run(RELEASE);
}


// SERVO SCAN

void scanSides(long &leftDist, long &rightDist) {

  scanServo.write(120);
  delay(250);
  leftDist = getDistance();

  scanServo.write(60);
  delay(250);
  rightDist = getDistance();

  scanServo.write(90);
  delay(150);
}


// AUTONOMOUS MODE

void autonomousMode() {

  long front = getDistance();
  Serial.println(front);

  if (front > detectDistance) {
    forward();
    return;
  }

  stopRobot();
  delay(80);

  backward();
  delay(250);
  stopRobot();
  delay(100);

  long leftDist, rightDist;
  scanSides(leftDist, rightDist);
  if (leftDist < tightSpace && rightDist < tightSpace) {
    while (true) {
      backward();
      delay(300);
      stopRobot();
      delay(150);
      scanSides(leftDist, rightDist);
      if (leftDist > tightSpace || rightDist > tightSpace) break;
    }
    if (leftDist > rightDist) {
      turnLeft();
      delay(500);
    } else {
      turnRight();
      delay(500);
    }

    stopRobot();
    return;
  }

  if (leftDist > rightDist) {
    turnLeft();
    delay(500);
  } else {
    turnRight();
    delay(500);
  }
  stopRobot();
}


// SETUP

void setup() {

  Serial.begin(9600);
  BT.begin(9600);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  scanServo.attach(10);
  scanServo.write(90);
  delay(300);

  stopRobot();
}

// LOOP
void loop() {

  // READ BLUETOOTH
  if (BT.available()) {
    char c = BT.read();

    // Mode switching
    if (c == 'A') currentMode = AUTONOMOUS;
    if (c == 'M') currentMode = MANUAL;

    // Manual commands
    if (c=='F' || c=='B' || c=='L' || c=='R' || c=='S') {

      manualCmd = c;
      unsigned long now = millis();

      if (c == 'S') {
        stopRobot();  // stop immediately
        manualCmdEndTime = now; 
      }
      else if (c == 'L' || c == 'R') {
        manualCmdEndTime = now + TURN_TIME;  // fixed 60° turning
      }
      else {
        manualCmdEndTime = now + MANUAL_MOVE_TIME; // forward/back for 0.3 sec
      }
    }
  }

  //  MANUAL MODE 
  if (currentMode == MANUAL) {

    unsigned long now = millis();

    if (now < manualCmdEndTime) {
      if (manualCmd == 'F') forward();
      else if (manualCmd == 'B') backward();
      else if (manualCmd == 'L') turnLeft();
      else if (manualCmd == 'R') turnRight();
    } 
    else {
      stopRobot();
    }

    return;
  }

  // ===== AUTONOMOUS MODE =====
  autonomousMode();
}
