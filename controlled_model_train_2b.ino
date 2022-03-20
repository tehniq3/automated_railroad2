/////////////////////////////////////////////////////
//
// Simple back and forth train control by Arduino with IR sensor
//
// ryuchihoon@gmail.com
// http://ardutrain.blogspot.com/2013/06/back-and-forth-train-control-by-arduino.html
// video: https://youtu.be/fNeRCVgfMRw
//
// ver.1.0 by niq_ro (Nicu Florica) - changed for usual IR module (HIGH - free, LOW - obstacle) and control a L298 module
// ver.1.b - PWM start from cca. 40% (105/255) not from 0
// ver.2.0 - added encoder and i2c 1602 display
// ver.2.b - force program to read just correct IR proximity sensor
//

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Encoder.h> //  http://www.pjrc.com/teensy/td_libs_Encoder.html

// train speed setups
//
// NOTE: Some kato trams are believed that their coreless motors are
//       fragile to low speed PWM'ed input.
//       I use normal(full) speed value as 255.
//
const int SPEED_MAX = 255;  // maximum PWM
const int SPEED_MIN = 105;  // minimum PWM for start 
const int SPEED_DELTA = 5;
//const int SPEED_MAX = 90;
//const int SPEED_DELTA = 8;


//
// pin conntection setups
//
#define IRD_A 7 // detector a
#define IRD_B 6  // detector b
#define LED_RED 13
#define SPEED_PWM 10
#define DIR 12
#define DIR2 11

#define CLK 2
#define DT 3
#define SW0 4
Encoder myEnc(DT, CLK);  // Best Performance: both pins have interrupt capability

LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int STAY_TIME = 4 * 1000;  // pause for change direction

const int CONTROL_INIT = 0;
const int CONTROL_DECEL = 1;
const int CONTROL_STOPPED = 2;
const int CONTROL_RUNNING = 3;
const int CONTROL_SLEEP = 4;
boolean ignore_detector_a = false;
boolean ignore_detector_b = false;
unsigned long time_to_leave = 0;
int train_control = CONTROL_INIT;
int train_speed = 0;
int train_dir1 = 1;
int train_dir2 = 0;  // negate train_dir
int train_dir0 = 0;  // stop

int viteza =0 ;
unsigned long tpfranare;
unsigned long tpafisare = 1500;


// https://www.arduino.cc/en/Reference/LiquidCrystalCreateChar
byte fata[8] = {
  B10000,
  B11000,
  B11100,
  B11111,
  B11100,
  B11000,
  B10000,
};

byte spate[8] = {
  B00001,
  B00011,
  B00111,
  B01111,
  B00111,
  B00011,
  B00001,
};

byte shtop[8] = {
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B00000,
  B00000,
};

byte starebuton0;             // the current reading from the input pin
byte citirestarebuton0;   // 
byte ultimastarebuton0 = HIGH;   // the previous reading from the input pin
unsigned long ultimtpdebounce0 = 0;  // the last time the output pin was toggled
unsigned long tpdebounce = 50;    // the debounce time; increase if the output flickers



void setup() {
  pinMode(IRD_A, INPUT);
  pinMode(IRD_B, INPUT);
  pinMode(SW0, INPUT);
  digitalWrite(IRD_A, HIGH);
  digitalWrite(IRD_B, HIGH);
  digitalWrite(SW0, HIGH);
  pinMode(LED_RED, OUTPUT);
  pinMode(SPEED_PWM, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(DIR2, OUTPUT);
  digitalWrite(DIR, LOW);
  digitalWrite(DIR2, LOW);

  lcd.init(); 
lcd.createChar(0, shtop);
lcd.createChar(1, fata);
lcd.createChar(2, spate);
                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print(" Sistem automat ");
  lcd.setCursor(0,1);
  lcd.print("control miscare ");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("   fata-spate   ");
  lcd.setCursor(0,1);
  lcd.print("   locomotiva   ");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("   analogica    ");
  lcd.setCursor(0,1);
  lcd.print("ver.2.0./niq_ro ");
  delay(2000);
lcd.clear();
  
}

void loop() {

citirestarebuton0 = digitalRead(SW0);  // read the state of the switch into a local variable: 
  if (citirestarebuton0 != ultimastarebuton0) // If the switch changed, due to noise or pressing:
  {
    ultimtpdebounce0 = millis();  // reset the debouncing timer
  }
  if ((millis() - ultimtpdebounce0) > tpdebounce) 
  {
    if (citirestarebuton0 != starebuton0) // if the button state has changed
    {
      starebuton0 = citirestarebuton0;         
      if (starebuton0 == LOW) // only toggle the LED if the new button state is LOW
      {
        train_dir0 = train_dir0 + 1;
        train_dir0 = train_dir0%2;
        if (train_dir0%2 == 1)
        {
        train_speed = 0;
        train_control = CONTROL_STOPPED;
        time_to_leave = millis() + STAY_TIME;
        }
      }
  }
  }

ultimastarebuton0 = citirestarebuton0;

if (train_dir0 == 0)  // stop
{
  train_control = CONTROL_SLEEP;
  if ((millis()/50)%25 ==1) digitalWrite(LED_RED, HIGH);
  else
  digitalWrite(LED_RED, LOW);
}
else
{
  int ir_detected = HIGH;  // free

  if (train_dir1 == 1) 
//  if(!ignore_detector_a) 
  {
    ir_detected = digitalRead(IRD_A);
     if(ir_detected == LOW) {
      ignore_detector_a = true;
      ignore_detector_b = false;
    }
  }
//  else if(!ignore_detector_b) 
  if (train_dir2 == 1) 
  {
    ir_detected = digitalRead(IRD_B);
    if(ir_detected == LOW) {
      ignore_detector_b = true;
      ignore_detector_a = false;
    }
  }

  if(ir_detected == LOW) 
  {
    train_control = CONTROL_DECEL;
    digitalWrite(LED_RED, HIGH);
    tpfranare = millis();
  }

  if(train_control != CONTROL_DECEL) 
  {
    digitalWrite(LED_RED, LOW);
  }

  if(train_control == CONTROL_INIT) 
  {
    train_speed = SPEED_MIN;
    train_control = CONTROL_RUNNING;
    digitalWrite(DIR, train_dir1);
    train_dir2 = (train_dir1+1)%2;
    digitalWrite(DIR2, train_dir2);
  }
}

speed_control();

lcd.setCursor(0,1);
lcd.print("Viteza:  ");

lcd.setCursor(12,1);
viteza = map(train_speed,0,255,0,100);
if (viteza < 100) lcd.print(" ");
if (viteza < 10) lcd.print(" ");
lcd.print(viteza);
lcd.print("%");

if (train_dir0 == 0)
{
  lcd.setCursor(0,0);
  lcd.print("STOP !  ");
}
else
 if (train_speed == 0) 
 {
  lcd.setCursor(0,0);
  lcd.print("Pauza ! ");
  lcd.setCursor(13,0);
  lcd.print(" ");
  lcd.setCursor(14,0);
  lcd.write(byte(0));
  lcd.setCursor(15,0);
  lcd.print(" ");
 }
 else
 {
  lcd.setCursor(0,0);
  if (millis() - tpfranare < tpafisare)
  lcd.print("Franare!");
  else
  lcd.print("Directie(");
lcd.setCursor(9,0);
lcd.print(train_dir1);
lcd.print("/");
lcd.print(train_dir2);
lcd.print(")");

  lcd.setCursor(14,0);
  if (train_dir1 == 0) 
 {
  lcd.write(byte(1));
  if (train_speed == 255) lcd.write(byte(1));
  else lcd.print(" ");
 }
  if (train_dir2 == 0) 
  {
  lcd.write(byte(2));
  lcd.setCursor(13,0);
  if (train_speed == 255) lcd.write(byte(2));
  else lcd.print(" ");
  }
 }


//train_speed = train_speed*train_dir0;
  delay(1);
} // end main loop


void speed_control() {
  switch(train_control) {
  case CONTROL_DECEL:
    train_speed -= SPEED_DELTA;
    if(train_speed <= SPEED_MIN) {
      train_speed = 0;
      train_control = CONTROL_STOPPED;
      time_to_leave = millis() + STAY_TIME;
    }
    break;
  case CONTROL_SLEEP:
    train_speed -= 5*SPEED_DELTA;
    if(train_speed <= SPEED_MIN) {
      train_speed = 0;
      digitalWrite(DIR, 0);
      digitalWrite(DIR2, 0);
    }
    break;  
  case CONTROL_STOPPED:
      digitalWrite(DIR, 0);
      digitalWrite(DIR2, 0);
    if(millis() > time_to_leave) {
   //   train_dir1 = HIGH - train_dir1;
      train_dir1 = (train_dir1+1)%2;
      train_dir2 = (train_dir1+1)%2;
      digitalWrite(DIR, train_dir1);
      digitalWrite(DIR2, train_dir2);
      train_control = CONTROL_RUNNING;
      train_speed = SPEED_MIN;
    }
    break;
  case CONTROL_RUNNING:
    train_speed += SPEED_DELTA;
    if(train_speed >= SPEED_MAX) {
      train_speed = SPEED_MAX;
    }
    break;
  }
  if(train_speed == 0) {
    digitalWrite(SPEED_PWM, LOW);
  } else if(train_speed == 255) {
    digitalWrite(SPEED_PWM, HIGH);
  } else {
    analogWrite(SPEED_PWM, train_speed);
  }
}

// EOF
