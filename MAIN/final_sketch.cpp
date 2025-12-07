#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Servo.h>

// LCD screen pins 
#define screen_CS    10
#define screen_DC     2   
#define screen_RST    4   
#define screen_BL     5   

Adafruit_ST7789 screen = Adafruit_ST7789(screen_CS, screen_DC, screen_RST);
//SRCS
//https://forums.adafruit.com/viewtopic.php?t=172241
//waveshare LCD modules 1.69 inch example code
//AS608 example code

int button_presses; 
int failed_attempts;


unsigned long time_since_lastunlock;


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
enum Mode { NORMAL, ENROLL };
Mode currentMode = NORMAL;


//Class 0 normal behavior
//Class 1 suspicious
//Class 2 bad


//Functions to log features for ML classification
/*
1. Log button presses  +++ reasoning - tampering , fraud 
2. Log time since unlock  +++ reasoning - tampering , fraud 
3. Log successful unlock , reset failed attempts and time since unlock  , +++ reasoning - tampering , fraud  security metrics , door should not be open for too long
  3 features -- failed attempts ,  button presses, time since unlock
*/


void increment_button_presses(){
button_presses++;
}

// LCD and serial printing
void showStatus(const char *line1,
                const char *line2 = "",
                const char *line3 = "") {
  // Print to serial
  Serial.println(F("---- STATUS ----"));
  Serial.println(line1);
  if (line2[0] != '\0') Serial.println(line2); // Handling empty lines
  if (line3[0] != '\0') Serial.println(line3);
  Serial.println(F("----------------"));

  // LCD
  screen.fillScreen(ST77XX_BLACK); // black bck
  screen.setTextColor(ST77XX_WHITE);
  screen.setTextSize(2); 

  screen.setCursor(5, 40); // upper left corner
  screen.print(line1);
  //screen length in thirds, 40px margin
  if (line2[0] != '\0') {
    screen.setCursor(5, 80);
    screen.print(line2);
  }
  if (line3[0] != '\0') {
    screen.setCursor(5, 120);
    screen.print(line3);
  }

  return;
}

void setup() {


  Serial.begin(9600); //Standard baud rate
  finger.begin(57600); // Defined in the datasheet for the module firmware ++ will not work o/w

//Make sure the fingerprint sensor is working
  if (!finger.verifyPassword()) { // Checks the default password and returns true , letting us know the sensor is wired correctly
    
    Serial.println("Fingerprint sensor not found!");
    while (1);
  }


   screen.init(240, 280);   // IMPORTANT init the screen

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
 


  /* //************
    CHECK FOR INPUT 
   */ //************
  HandleButtons();

  if (currentMode == ENROLL) {
    showStatus("Starting","enrollment", "mode");
    performEnrollment();
    delay(500);
  } else {
      
    // NORMAL MODE: Scan to unlock
    int id = getFingerprintID();
    if (id >= 0) {
     showStatus("Match", "found", "welcome");
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
    currentMode = ENROLL;
    delay(500);
  }

  if (digitalRead(B_NORMAL) == LOW) {
    Serial.println("Normal Mode button pressed → Returning to NORMAL MODE");
    currentMode = NORMAL;
    delay(500);
  }

  increment_button_presses();

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
    currentMode = NORMAL;
    started = false;
    return;
  }

  if (enrollFingerprint(nextID)) {
    Serial.print("Enrollment successful! Stored at ID: ");
    showStatus("Enrollment  ", "successful", "!");
    Serial.println(nextID);
  } else {
    Serial.println("Enrollment failed.");
       showStatus("enrollment  ", "was", "unsuccessful");
  }


  started = false;
  currentMode = NORMAL;
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

bool enrollFingerprint(int id) {
  int p;

  Serial.print("Waiting for finger for ID ");
  Serial.println(id);
      showStatus("Place finger to begin");
  // Scan finger 1st attempt
  while ((p = finger.getImage()) != FINGERPRINT_OK ){
    //Normal mode button interupt , this is so if we choose to stop enrollment at any time, we can
    if(digitalRead(B_NORMAL) == LOW){
      currentMode = NORMAL;
      return false;
    }
  };

  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;
  Serial.println("First scan OK, remove finger...");

  delay(2000);

  // scan finger 2nd attempt
  Serial.println("Place same finger again...");
      showStatus("Place same finger again");
  while ((p = finger.getImage()) != FINGERPRINT_OK );


  //Flow according to documentation
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message
 
  if (finger.createModel() != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message

  if (finger.storeModel(id) != FINGERPRINT_OK) return false;
  //TODO Add debug message + on screen message
      showStatus("Success");
  return true;
}

/*
Data from offline logistic regression classifier training

Weights ->>>  [[2.55e+00 1.88e+00 6.19e-04]]
B = Bias ->>>>  [-9.87]

z = B + W1X1 + W2X2 + W3X3

sigmoid func =  1/(1 + e^-z)
*/
bool validate_data(int x1, int x2, int x3){


  float bias = -9.87;
  float w1 = 2.55;
  float w2 = 1.88;
  float w3 = 6.19; 


  float z = bias + w1 * x1 + w2*x2 + w3 * x3;


  float logi = 1.0 / (1.0  + (exp(-(double)z)));




}

/*/
Display image  : we input a message and display it to the LCD screen
*/

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
  // o/w

  unsigned long validate_time = millis() - time_since_lastunlock; // How long has it been since last unlock
  

  //success

  //RESET on successful unlock
  button_presses = 0;
  failed_attempts = 0;
  time_since_lastunlock = 0;

}
