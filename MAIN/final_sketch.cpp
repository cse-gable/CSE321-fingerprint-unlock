#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Servo.h>

SoftwareSerial fingerSerial(6, 7); // RX = RD , TX = TD <----- source of many issues
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

Servo doorServo;

const int P_SERVO = 9;
const int OPEN_POSITION = 90;
const int CLOSED_POSITION = 0;
const unsigned long AUTO_CLOSE_TIME = 3000;

const int B_ENROLL = 12;
const int B_NORMAL = 3;

// States
enum Mode { MODE_NORMAL, MODE_ENROLL };
Mode currentMode = MODE_NORMAL;

void setup() {
  Serial.begin(9600); //Standard baud rate
  finger.begin(57600); // Defined in the datasheet for the module firmware ++ will not work o/w

//Make sure the fingerprint sensor is working
  if (!finger.verifyPassword()) { // Checks the default password and returns true , letting us know the sensor is wired correctly
    
    Serial.println("Fingerprint sensor not found!");
    while (1);
  }

  doorServo.attach(P_SERVO);
  doorServo.write(CLOSED_POSITION);
  ///Setup buttons
  pinMode(B_ENROLL, INPUT_PULLUP);
  pinMode(B_NORMAL, INPUT_PULLUP);

  Serial.println("System ready (NORMAL MODE)");
}



/*
 MAIN

*/
void loop() {
 


  /* /************
    CHECK FOR INPUT 
    

   */ //************
  HandleButtons();

  if (currentMode == MODE_ENROLL) {
    performEnrollment();
  } else {
    // NORMAL MODE: Scan to unlock
    int id = getFingerprintID();
    if (id >= 0) {
      Serial.print("Match: ID ");
      //TODO output to LCD that we have found a match
      Serial.println(id);
      unlockDoor();
    }
  }

  delay(50);
}

/*
checking button feedback
NOTE switch from high to low
++ added 10k resistors
*/
void HandleButtons() {
  if (digitalRead(B_ENROLL) == LOW) {
    Serial.println("Enroll button pressed → Entering ENROLL MODE");
    currentMode = MODE_ENROLL;
    delay(500);
  }

  if (digitalRead(B_NORMAL) == LOW) {
    Serial.println("Normal Mode button pressed → Returning to NORMAL MODE");
    currentMode = MODE_NORMAL;
    delay(500);
  }
}



/*
Enrollment handling
*/

void performEnrollment() {
  static bool started = false;

  if (!started) {
    Serial.println("=== ENROLL MODE ===");
    Serial.println("Place finger to enroll...");
    started = true;
  }

  int nextID = findNextEmptyID();
  if (nextID < 0) {
    Serial.println("Sensor full! Returning to NORMAL MODE.");
    currentMode = MODE_NORMAL;
    started = false;
    return;
  }

  if (enrollFingerprint(nextID)) {
    Serial.print("Enrollment successful! Stored at ID: ");
    Serial.println(nextID);
  } else {
    Serial.println("Enrollment failed.");
  }


  started = false;
  currentMode = MODE_NORMAL;
}

// Finds next available slot (0–127 typically)
int findNextEmptyID() {
  for (int id = 1; id < 127; id++) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id;
    }
  }
  return -1;
}
//Handles the enrollment of fingerprints 

bool enrollFingerprint(int id) {
  int p;

  Serial.print("Waiting for finger for ID ");
  Serial.println(id);

  // Scan finger 1st attempt
  while ((p = finger.getImage()) != FINGERPRINT_OK ){
    //Normal mode button interupt , this is so if we choose to stop enrollment at any time, we can
    if(digitalRead(B_NORMAL) == LOW){
      currentMode = MODE_NORMAL;
      return false;
    }
  };

  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;
  Serial.println("First scan OK, remove finger...");

  delay(2000);

  // scan finger 2nd attempt
  Serial.println("Place same finger again...");
  while ((p = finger.getImage()) != FINGERPRINT_OK );


  //Flow according to documentation
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message
 
  if (finger.createModel() != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message

  if (finger.storeModel(id) != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message
  return true;
}

/*
This function gets the finger image and returns an id if it was successful

IF there is no match or an error then we expect to return -1 indicating failure.

*/
int getFingerprintID() {
  finger.getImage();
  if (finger.image2Tz() != FINGERPRINT_OK) return -1;
  if (finger.fingerFastSearch() != FINGERPRINT_OK) return -1;
  return finger.fingerID;
}

void unlockDoor() {
  doorServo.write(OPEN_POSITION);
  Serial.println("Door OPEN");

  delay(AUTO_CLOSE_TIME);
  // Real time embedded systems stipulation
  //TODO ADD MSG on screen
  doorServo.write(CLOSED_POSITION);
  Serial.println("Door CLOSED");
}
