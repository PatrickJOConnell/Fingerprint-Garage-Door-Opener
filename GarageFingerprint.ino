#include <LiquidCrystal.h>      // LCD Library
#include <Stepper.h>            // Stepper motor library
#include "FPS_GT511C3.h"        // Fingerprint sensor library
#include "SoftwareSerial.h"     // Software serial library used for serial communication with the fingerprint sensor

#define revolution_steps 32


const int RS = 7, E = 8, D4 = 9, D5 = A1, D6 = A0, D7 = 12;  // LCD output Pins

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);                    // Initialize the LCD             
FPS_GT511C3 fps(10, 11);                                     // Initialize the FPS with software serial pins  

Stepper garageMotor(revolution_steps, 3, 5, 4, 6);

int  steps;
int motorDirection = 1;
double timeStart = 0, timeEnd = 0, timeDifferential = 0;
int buttonState = 0;
boolean touchState = 0;
int lag = 0;
int touchLag = 0;
int state = 0;
int garageopen = 0;                       // Garage open/closed status
int i = 0;                                // Iterator variable
int inc = 0;                              // Variable used for incoming serial data
boolean Direction = true;

void setup() {
  Serial.begin(9600);

  //Initializing the Libraries
  lcd.begin(16, 2);

  //Print the status of the garage
  lcd_print("Garage Opener   ", "Garage: Closed  ");
  fps.Open();
  fps.SetLED(true);

  //Set Outputs
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
}


void openclosegarage() {
  steps  =  4096;
  garageMotor.setSpeed(1000);
  garageMotor.step((motorDirection * steps));
  motorDirection = motorDirection * -1;
}

void lcd_print(String topText, String bottomText) {   // Given two strings, prints the strings on the LCD
  lcd.setCursor(0, 0);                                // Set the cursor to the first row
  lcd.print(topText);
  lcd.setCursor(0, 1);                                // Set the cursor to the second row
  lcd.print(bottomText);
}

/**
   Prints the current status of the garage panel
*/
void openclose() {
  garageopen = !garageopen;
  if (garageopen) {
    lcd_print("Garage Opener   ", "Garage: Opening ");  // Shows that garage is opening when the garage is opening
    delay(2000);
    openclosegarage();
    lcd_print("Garage Opener   ", "Garage: Open    ");  // Display states that the garage is open
  }
  else {
    lcd_print("Garage Opener   ", "Garage: Closing ");  // Shows that garage is closing when the garage is closing
    delay(2000);
    openclosegarage();
    lcd_print("Garage Opener   ", "Garage: Closed  ");  // Display states that the garage is closed
  }
}

/**

*/
void loop() {
  while (Serial.available() > 0) {      // Check for serial communications from the GarageMotorRunner module
    inc = Serial.read();
    if (inc == 1) {
      openclose();                      // Open/close the garage if given the serial value of 1
    }
    if (inc == 2) {
      Register_Finger();                // Enroll a finger when sent the 2 signal
    }
    if (inc == 3) {
      boolean del = fps.DeleteAll();    // Delete all fingerprints when sent a value of 3
      if (del) {
        lcd_print("    Success.    ", "All IDs deleted ");   // All IDs have been deleted
      }
      else {
        lcd_print("    Failure.    ", " No IDs deleted ");   // No IDs have been deleted (may not be any IDs to delete)
      }
      delay(5000);                                           // Give the user 5 seconds to read the message
    }
  }

  
  if (garageopen) {     // Print the status of the gargae (open)
    lcd_print("Garage Opener   ", "Garage: Open    ");
  }
  else {                // Print the status of the gargae (closed)
    lcd_print("Garage Opener   ", "Garage: Closed  ");
  }

  touchState = fps.IsPressFinger();   // Check to see if a finger is pressed on the sensor
  if (touchState && !touchLag) {      // check for a rising edge
    touchLag = 1;
    timeStart = millis();             // being a timer once the fingerprint sensor is touched for the first time
    while (touchState) {
      touchState = fps.IsPressFinger();
      timeEnd = millis();
      timeDifferential = timeEnd - timeStart;
      if (timeDifferential == 100) {  // if the sensor has been touched continuously for the last 100 ms

        bool cap =  fps.CaptureFinger(false); //Capture the fingerprint for identification
        int id_number = fps.Identify1_N();   //Identify if the fingerprint is stored in the sensor (returns 200 when Id not stored)
        if (cap && id_number != 200) {

          if (id_number == 0) {             //Known guests have custom LCD Screen greetings
            lcd_print("    Welcome     ", "    Patrick     ");
          }

          else if (id_number < 10) {        //Uncostomized guests have LCD Screen greeting with their ID number value
            lcd_print("    Welcome     ", ("Guest #" + String(id_number) + "        "));
          }
          else if (id_number < 100) {
            lcd_print("    Welcome     ", ("Guest #" + String(id_number) + "       "));
          }
          else {
            lcd_print("    Welcome     ", ("Guest #" + String(id_number) + "      "));
          }
          Serial.write(1);
          delay(2500);
          openclose();
          break;
        }
        else {

         
          lcd_print(" Access Denied  ", "                ");   // If the authentications fails in any way - print failure message (unverified finger)
          delay(2500);                                         // Print the access denied message for 2.5 seconds
          break;
        }
      }
    }
  }
  else if (!touchState && touchLag) {                          // Reset the variable used to find the rising signal edge
    touchLag = 0;
  }
}

/**

*/
void finger_delay(bool fingerPressed) { // Function called when the enrollment process is underway
  while (fps.IsPressFinger() == fingerPressed) delay(100);    // Wait 100 ms after a finger is sensed
}

void Register_Finger() {
  int finger_ID_number = 0;                                   // Start user ID numbers at 0
  bool ID_number_in_use = true;
  while (ID_number_in_use == true) {                          // Iterate through used user IDs to find the next open ID number
    ID_number_in_use = fps.CheckEnrolled(finger_ID_number); 
    if (ID_number_in_use == true) finger_ID_number++;
  }
  fps.EnrollStart(finger_ID_number);                          // Call the Enroll start function from the FPS library

  lcd_print("Place the finger", "to enroll on pad");          // First placement of the finger to enroll
  while (fps.IsPressFinger() == false) delay(100);
  bool finger_capture_boolean = fps.CaptureFinger(true);
  int store_success_int = 0;
  if (finger_capture_boolean != false) {                      // Fingerprint is acceptable for storage
    lcd_print("Remove finger...", "                ");
    fps.Enroll1();
    while (fps.IsPressFinger() == true) delay(100);
    lcd_print("Place the same  ", "finger again    ");        // Second placement of the finger to enroll
    while (fps.IsPressFinger() == false) delay(100);
    finger_capture_boolean = fps.CaptureFinger(true);
    if (finger_capture_boolean != false)                      // Fingerprint is acceptable for storage
    {
      lcd_print("Remove finger...", "                ");
      fps.Enroll2();
      while (fps.IsPressFinger() == true) delay(100);

      //Third enroll
      lcd_print("Last time...    ", "place finger    ");      // Third placement of the finger to enroll
      while (fps.IsPressFinger() == false) delay(100);        
      finger_capture_boolean = fps.CaptureFinger(true);
      if (finger_capture_boolean != false) {                  // Fingerprint is acceptable for storage
        lcd_print("Remove finger...", "                ");
        store_success_int = fps.Enroll3();
        if (store_success_int == 0) {                         // Message diplayed when finger was successfully enrolled
          lcd_print("  Fingerprint   ", "    Stored!     ");
        }
        else {                                               
          lcd_print("Failed to store.", "   Try again.   ");  
        }
      }
      else {                                                  // Message diplayed when finger was not successfully enrolled (third placement)
        lcd_print("Failed to store.", "   Try again.   ");
      }
    }
    else {                                                    // Message diplayed when finger was not successfully enrolled (second placement)
      lcd_print("Failed to store.", "   Try again.   ");
    }
  }
  else {                                                      // Message diplayed when finger was not successfully enrolled (first placement)
    lcd_print("Failed to store.", "   Try again.   ");
  }
  delay(3000);
}

