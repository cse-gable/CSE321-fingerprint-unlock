# CSE321-fingerprint-unlock

Deployement 

You will need some familiarity with arduino and wiring. In addition to the hardware components outline you will need a breadboard and several jumper wires to follow the deployment stage.
** Note ** 
If you take a look at the main script, the digital input pins used for the different componenets are detailed. Other than what is explicity mentioned(input pins 11 and 13) it doesnt matter what pins are used.

Connect the 1.69 inch waveshare LCD screen to ardunio, make sure the RST and DIN pins are on 11 and 13 respectively. The rest of pins just need to plugged in on any of the digit input pins for the arduino.
// LCD screen pins  
RST 11 and DIN 13
#define screen_CS    10
#define screen_DC     2   
#define screen_RST    4   
#define screen_BL     5   

Connect the servo motor to the arduino, 5V pin to the breadboard line where you plug the arduino 5V pin so that they are wired in series.
Connect the AS608 fingerprint module to the arduino as desired.
Connect two tacticle buttons to the arduino and breadboard as desired. One button will be for the enrollment mode and one with be for the normal mode regular lock/unlock functionality.


Here is my configuration for reference 
![IMG_1521](https://github.com/user-attachments/assets/6588ac25-a064-4b44-bf3d-b84ac633fcac)

Project use

1. Start by pressing the enrollment mode button. Follow the on screen instructions . 
2. Press the normal mode button to return to the normal mode, from there you can use the fingerprint module with the previously enrolled finger to unlock the door via the servo motor.

   You can enroll up to 100 fingerprints on the module.
   This device is meant as a prototype and proof of concept for a fingerprint verifcation lock for a door.
