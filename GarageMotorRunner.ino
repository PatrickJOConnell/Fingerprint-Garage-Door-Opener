// Authors: Patrick O'Connell and Jahnvi Patel
// University of Illinois at Chicago 2018

#include <Stepper.h>          // stepper motor library


#define revolution_steps 32   // Steps for garagemotor initialization
#define button_1 2            // When this button is held down, the garage will close 
#define button_2 12           // When this button is held down, the garage will open
#define button_3 13           // Button to toggle open/close door, enroll fingerprints, and delete all fingerprints

Stepper garageMotor(revolution_steps, 3, 5, 4, 6);
Stepper garageMotor2(revolution_steps, 8, 10, 9, 11);

int  steps = 6100;            // The number of steps that the both motors will step upon the open or close command
int motorDirection = 1;       // The motor direction vaiable is toggled after every successful open or close
double timeStart = 0, timeEnd = 0, timeDifferential = 0;
int buttonState = 0;
int buttonState2 = 0;
int buttonState3 = 0;
boolean touchState = 0;       // 1 when fingerprint sensor is being touched
int lag = 0;                  // Variable used for rising signal edge detection
int lag2 = 0;                 // Variable used for rising signal edge detection
int touchLag = 0;             // Variable used for rising signal edge detection
int state = 0;
int garageopen = 0;           // 0 when garage closed, 1 when garage open
int i = 0;                    // iteration variable
int inc = 0;
boolean Direction = true;

void setup() {
  
  Serial.begin(9600);
  pinMode(button_1, INPUT);
  pinMode(button_2, INPUT);
  pinMode(button_3, INPUT);
  
  pinMode(3, OUTPUT);         //Motor 1 outputs
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  pinMode(8, OUTPUT);         //Motor 2 outputs
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
}


void opengarage() {
  garageMotor.setSpeed(1000);         // The higher the value, the faster the motor (1000 is near the max speed)
  garageMotor.step(1);                // motor one steps in one direction
  garageMotor2.setSpeed(1000);
  garageMotor2.step(-1);              // motor 2 steps in the opposite direction
}


void closegarage() {
  garageMotor.setSpeed(1000);
  garageMotor.step(-1);               // motor one steps in one direction
  garageMotor2.setSpeed(1000);
  garageMotor2.step(1);               // motor two steps in the opposite direction
}

void loop() {
  while (Serial.available() > 0) {    // check for serial input from the fingerprint module
    inc = Serial.read();
    if (inc == 1) {                   // an open/close request has been sent
      if (garageopen) {
        for (i = 0; i < steps; i++) {
          closegarage();
        }
      }
      else {
        for (i = 0; i < steps; i++) {
          opengarage();
        }
      }
      garageopen = !garageopen;
    }
  }

  buttonState  = digitalRead(button_1);   // record the open, close, and enroll/delete button states
  buttonState2 = digitalRead(button_2);
  buttonState3 = digitalRead(button_3);

  if (buttonState3 && !lag2) {            //check for rising edge on the enroll/delete button
    lag2 = 1;
    timeStart = millis();
    while (buttonState3) {
      buttonState3 = digitalRead(button_3);
    }
    timeEnd = millis();
    timeDifferential = timeEnd - timeStart;
    if (timeDifferential > 10000) {       // if held for > 10 seconds, send request to the fingerprint module to delete all fingerprints
      Serial.write(3);
    }
    else if (timeDifferential > 3000) {   // if held for < 10 seconds and > 3 seconds, begin the finger enrollment
      Serial.write(2);
      //Enroll();
    }
    else {
      Serial.write(1);                    // if held for less than 3 seconds, open or close the door depending on the garage state
      if (garageopen) {
        for (i = 0; i < steps; i++) {
          closegarage();
        }
      }
      else {
        for (i = 0; i < steps; i++) {
          opengarage();
        }
      }
      garageopen = !garageopen;           // toggle the garage state after opening or closing
    }
  }
  else if (!buttonState3 && lag2) {
    lag2 = 0;
  }

  if (buttonState) {                      // open the garage for as long as this button is pressed
    opengarage();
  }
  if (buttonState2) {                     // close the garage for as long as this button is pressed
    closegarage();
  }
}
