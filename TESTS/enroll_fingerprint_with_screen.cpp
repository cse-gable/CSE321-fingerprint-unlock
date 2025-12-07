#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// LCD screen pins 
#define screen_Cs    10
#define screen_DC     2   // DC -> D2
#define screen_RST    4   // RST -> D4
#define screen_BL     5   // BL  -> D5

Adafruit_ST7789 screen = Adafruit_ST7789(screen_Cs, screen_DC, screen_RST);
//SRCS
//https://forums.adafruit.com/viewtopic.php?t=172241
//waveshare LCD modules 1.69 inch example code
//AS608 example code



#define FP_RX   6   // Arduino RX  -> sensor TD
#define FP_TX   7   // Arduino TX  -> sensor RD (through level divider if needed)

SoftwareSerial sensorSerial(FP_RX, FP_TX);      // (RX, TX)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&sensorSerial);


uint8_t enrollID = 1;

// LCD and serial printing
void showStatus(const char *line1,
                const char *line2 = "",
                const char *line3 = "") {
  // Print to serial
  Serial.println(F("---- STATUS ----"));
  Serial.println(line1);
  if (line2[0] != '\0') Serial.println(line2);
  if (line3[0] != '\0') Serial.println(line3);
  Serial.println(F("----------------"));

  // LCD
  screen.fillScreen(ST77XX_BLACK); // black bck
  screen.setTextColor(ST77XX_WHITE);
  screen.setTextSize(2); 

  screen.setCursor(5, 40); // upper left corner
  screen.print(line1);

  if (line2[0] != '\0') {
    screen.setCursor(5, 80);
    screen.print(line2);
  }
  if (line3[0] != '\0') {
    screen.setCursor(5, 120);
    screen.print(line3);
  }
}


void setStatusLED(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}


bool enrollFinger(uint8_t id) {
  int p = -1;
  char idBuf[16];
  sprintf(idBuf, "ID %d", id);

  // Ready: LED off
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
  setStatusLED(false);

  // --- First scan ---
  showStatus("Enroll finger", idBuf, "Place finger");
  Serial.println(F("Waiting for first finger..."));

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();

    if (p == FINGERPRINT_NOFINGER) {
      // nothing yet
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      showStatus("Sensor error");
      Serial.println(F("Packet receive error (first scan)"));
      return false;
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      showStatus("Imaging error");
      Serial.println(F("Imaging error (first scan)"));
      return false;
    }
    delay(50);
  }

  // Finger detected: set LED ON
  setStatusLED(true);
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
  Serial.println(F("First image taken"));

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    showStatus("img 1 fail");
    Serial.print(F("image2Tz(1) error: ")); Serial.println(p);
    return false;
  }
  Serial.println(F("First image converted"));

  // Ask user to remove
  showStatus("Remove finger");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
  Serial.println(F("Remove finger..."));
  delay(1000);

  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    // wait for finger to be removed
    delay(50);
  }
  Serial.println(F("Finger removed"));
  setStatusLED(false);

  // --- Second scan ---
  showStatus("Place same", "finger again");
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
  Serial.println(F("Waiting for same finger"));

  p = -1; //error 2
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
//wait
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      showStatus("Sensor error");
      Serial.println(F("error2"));
      return false;
    } else if (p == FINGERPRINT_IMAGEFAIL) {
      showStatus("Imaging error");
      Serial.println(F("Imaging error (second scan)"));
      return false;
    }
    delay(50);
  }

  setStatusLED(true);
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
  Serial.println(F("Second image taken"));

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    showStatus(" img 2 fail");
    Serial.print(F("img error: ")); Serial.println(p);
    return false;
  }
  Serial.println(F("Second image converted corrctly"));

  // --- Create and store model ---
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    showStatus("Match failed");
    Serial.print(F("createModel error: ")); Serial.println(p);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
    setStatusLED(false);
    return false;
  }
  Serial.println(F("Fingerprints matched"));

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    showStatus("Enroll OK!", idBuf);
    Serial.println(F("Stored fingerprint successfully"));
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
    setStatusLED(true);
    return true;
  } else {
    showStatus("Store failed");
    Serial.print(F("storeModel error: ")); Serial.println(p);
    finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
    setStatusLED(false);
    return false;
  }
}


void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatusLED(false);

  Serial.begin(115200);
  while (!Serial) { ; }

  Serial.println();
  Serial.println(F("LCD + Fingerprint ENROLL UI"));


  pinMode(screen_BL, OUTPUT);
  digitalWrite(screen_BL, HIGH);  // backlight ON

  // LCD init
  screen.init(240, 280);   // 240x280 screen size
  screen.setRotation(0);
  showStatus("LCD ready");

  // Fingerprint init
  finger.begin(57600);  // use the baud that worked for you
  delay(200);

  Serial.println(F("Checking fingerprint sensor..."));
  if (!finger.verifyPassword()) {
    showStatus("No sensor!", "Check wiring");
    Serial.println(F("Fingerprint sensor NOT found."));
    Serial.println(F("Check TD/RD, VCC, GND, and baud=57600."));
    while (1) { delay(10); }
  }
  showStatus("Fingerprint", "sensor OK");
  Serial.println(F("Fingerprint sensor verified!"));

  // Show current template count
  int rc = finger.getTemplateCount();
  if (rc == FINGERPRINT_OK) {
     Serial.print(F("Templates on device: "));
    Serial.println(finger.templateCount);
  } else {
    Serial.println(F("Could not read template count (still OK)."));
  }

  delay(2000);
  char idBuf[16];

    sprintf(idBuf, "ID %d", enrollID);
  showStatus("Ready to", "enroll finger", idBuf);

  setStatusLED(false);
  Serial.print(F("Ready to enroll ID ")); Serial.println(enrollID);
}


void loop() {
  static bool done = false;

  if (!done) {
    bool ok = enrollFinger(enrollID);
    done = true;

    if (!ok) {
      showStatus("Enroll failed", "Reset to retry");
      finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
      setStatusLED(false);
      Serial.println(F("Enrollment failed. Reset to try again."));
    } else {
      Serial.println(F("Enrollmnt completed successfully."));
    }
  }


}
