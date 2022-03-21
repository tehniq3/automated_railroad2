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
// ver.2.0 - add encoder (just main switch) and i2c 1602 display
// ver.2.b - force program to read just correct IR proximity sensor
// ver.2.c - clean info on display in STOP 
// ver.2.d - add information in english (until than all info was in romanian language)
//           + improve IR detection (ignore detection at speed is zero
// ver.3.0 - add encoder feature - menu

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Encoder.h> //  http://www.pjrc.com/teensy/td_libs_Encoder.html
#include <EEPROM.h>

// train speed setups
//
// NOTE: Some kato trams are believed that their coreless motors are
//       fragile to low speed PWM'ed input.
//       I use normal(full) speed value as 255.

int SPEED_MAX = 255;  // 255 - maximum PWM
int SPEED_MIN = 25;  // 105 - minimum PWM for start (for tests value can be lower, e.g.25)
int SPEED_DELTA = 5;

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
Encoder knob(DT, CLK);  // Best Performance: both pins have interrupt capability

LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

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
int romana = 0;  // 1 for romana, 0 for english

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

// These variables are for the push button routine
int buttonstate = 0; //flag to see if the button has been pressed, used internal on the subroutine only
int pushlengthset = 3000; // value for a long push in mS
int pushlength = pushlengthset; // set default pushlength
int pushstart = 0;// sets default push value for the button going low
int pushstop = 0;// sets the default value for when the button goes back high
int durata1 = 0;

int knobval; // value for the rotation of the knob
boolean buttonflag = false; // default value for the button flag

//the variables provide the holding values for the set variables
int setlimbatemp; 
int setirpostemp;
int irpos = 0;  // 0 - normal, 1 - reverse
int setstaysectemp;
int staysec = 4;
int setspeeddeltatemp;
int setspeedmintemp;
int setspeedmaxtemp;
int setdirtemp;
int directie = 0;  // 0 - normal, 1 - reverse

int STAY_TIME = staysec * 1000;  // pause for change direction

byte checkok = 9;  // value for check the eeprom state
byte adresa = 200; // adresa initiala de memorare

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

  if (EEPROM.read(adresa) == checkok) 
{
  romana = EEPROM.read(adresa+1);
  directie = EEPROM.read(adresa+2);
  irpos = EEPROM.read(adresa+3);
  staysec = EEPROM.read(adresa+4);
  SPEED_MAX = EEPROM.read(adresa+5);
  SPEED_MIN = EEPROM.read(adresa+6);
  SPEED_DELTA = EEPROM.read(adresa+7);
}
else
{
  EEPROM.update(adresa, checkok);
  EEPROM.update(adresa+1, romana);
  EEPROM.update(adresa+2, directie);
  EEPROM.update(adresa+3, irpos);
  EEPROM.update(adresa+4, staysec);
  EEPROM.update(adresa+5, SPEED_MAX);
  EEPROM.update(adresa+6, SPEED_MIN);
  EEPROM.update(adresa+7, SPEED_DELTA);
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
}
 STAY_TIME = staysec * 1000; 
  
  lcd.init(); 
lcd.createChar(0, shtop);
lcd.createChar(1, fata);
lcd.createChar(2, spate);
                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0);
  if (romana == 1) 
  {
  lcd.print(" Sistem automat ");
  lcd.setCursor(0,1);
  lcd.print("control miscare ");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("fata-spate loco-");
  lcd.setCursor(0,1);
  lcd.print("motiva analogica");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("program original");
  lcd.setCursor(0,1);
  lcd.print("ver.3.0./niq_ro ");
  }
  else
  {
  lcd.print("Automatic system");
  lcd.setCursor(0,1);
  lcd.print("movement control");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("forward-backward");
  lcd.setCursor(0,1);
  lcd.print("  analog loco   ");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("  original sw.  ");
  lcd.setCursor(0,1);
  lcd.print("by niq_ro /v.3.0");
  }
  delay(2000);
lcd.clear();
}

void loop() {

pushlength = pushlengthset;
    pushlength = getpushlength ();
    delay (10);
    
    if (pushlength < pushlengthset) // short push
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
    
       
       //This runs the setclock routine if the knob is pushed for a long time
       if ((pushlength > pushlengthset) and (train_speed == 0))
       {
         lcd.clear();
         meniu();
         pushlength = pushlengthset;
       };


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

if (irpos == 0)
{
  if ((train_dir1 == 1) and (train_speed != 0))
   {
    ir_detected = digitalRead(IRD_A);
   } 
  if ((train_dir2 == 1) and (train_speed != 0))
   {
    ir_detected = digitalRead(IRD_B);
   }
}
else
{
    if ((train_dir2 == 1) and (train_speed != 0))
   {
    ir_detected = digitalRead(IRD_A);
   } 
  if ((train_dir1 == 1) and (train_speed != 0))
   {
    ir_detected = digitalRead(IRD_B);
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

if (train_dir0 == 0)
{
  lcd.setCursor(0,0);
  lcd.print("STOP !   ");
  lcd.setCursor(8,0);
  lcd.print("         "); 
  lcd.setCursor(0,1);
  lcd.print("                ");
}
else
{
lcd.setCursor(0,1);
if (romana == 1) 
   lcd.print("Viteza:  ");
else 
   lcd.print("Speed:   ");
lcd.setCursor(12,1);
viteza = map(train_speed,0,255,0,100);
if (viteza < 100) lcd.print(" ");
if (viteza < 10) lcd.print(" ");
lcd.print(viteza);
lcd.print("%");
  
 if (train_speed == 0) 
 {
  lcd.setCursor(0,0);
if (romana == 1) 
  lcd.print("Pauza !  ");
else 
  lcd.print("Pause !  "); 
  lcd.setCursor(13,0);
  lcd.print(" ");
  lcd.setCursor(14,0);
  lcd.write(byte(0));
  lcd.setCursor(15,0);
  lcd.print(" ");
lcd.setCursor(8,0);
lcd.print("     "); 
 }
 else
 {
  lcd.setCursor(0,0);
  if (millis() - tpfranare < tpafisare)
  {
if (romana == 1) 
   lcd.print("Franare! ");
else 
   lcd.print("Brake !  ");  
  }
  else
  {
if (romana == 1) 
   lcd.print("Directie (");
else 
   lcd.print("Direction(");
  }
lcd.setCursor(10,0);
lcd.print(train_dir1);
//lcd.print("/");
lcd.print(train_dir2);
lcd.print(")");

  lcd.setCursor(14,0);
//
if (directie == 0)
{
  if (train_dir1 == 0) 
 {
  lcd.write(byte(1));
  if (train_speed == SPEED_MAX) lcd.write(byte(1));
  else lcd.print(" ");
 }
  if (train_dir2 == 0) 
  {
  lcd.write(byte(2));
  lcd.setCursor(13,0);
  if (train_speed == SPEED_MAX) lcd.write(byte(2));
  else lcd.print(" ");
  }
}
else
{
  if (train_dir2 == 0) 
 {
  lcd.write(byte(1));
  if (train_speed == SPEED_MAX) lcd.write(byte(1));
  else lcd.print(" ");
 }
  if (train_dir1 == 0) 
  {
  lcd.write(byte(2));
  lcd.setCursor(13,0);
  if (train_speed == SPEED_MAX) lcd.write(byte(2));
  else lcd.print(" ");
  }
 } 
 }
}

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


// subroutine to return the length of the button push.
int getpushlength() 
{
  buttonstate = digitalRead(SW0);  
       if(buttonstate == LOW && buttonflag==false) 
          {     
              pushstart = millis();
              buttonflag = true;
          };
          
       if (buttonstate == HIGH && buttonflag==true) 
          {
         pushstop = millis ();
         pushlength = pushstop - pushstart;
         buttonflag = false;
         lcd.setCursor (15,1);
         lcd.print (" ");
          };
       
       if(buttonstate == LOW && buttonflag == true) // https://nicuflorica.blogspot.com/2020/04/indicare-apasare-buton-apasare-scurta.html
          {  // if button is still push
          durata1 = millis() - pushstart;
          if (durata1 > pushlengthset) 
          {   
           lcd.setCursor (15,1);
           lcd.print ("*");
          }
          }
       return pushlength;
}

//menu
void meniu ()
{
  setlimbatemp = romana;
  setdirtemp = directie;
  setirpostemp = irpos;
  setstaysectemp = staysec;
  setspeedmaxtemp = SPEED_MAX;
  setspeedmintemp = SPEED_MIN;
  setspeeddeltatemp = SPEED_DELTA;
   
   setlimba();  // language
   lcd.clear();
   setdir();  // direction (normal or reverse)
   lcd.clear();
   setirpos();  // IR sensor activation (normal or reverse)
   lcd.clear();
   setstaysec(); // time to stay in pause
   lcd.clear();
   setspeedmax();
   lcd.clear();
   setspeedmin();
   lcd.clear();
   setspeeddelta();
   lcd.clear();
   delay (1000); 

romana = setlimbatemp;
irpos = setirpostemp;
staysec = setstaysectemp;
STAY_TIME = staysec * 1000;
SPEED_MAX = setspeedmaxtemp;
SPEED_MIN = setspeedmintemp;
SPEED_DELTA = setspeeddeltatemp;  
directie = setdirtemp; 
  EEPROM.update(adresa, checkok);
  EEPROM.update(adresa+1, romana);
  EEPROM.update(adresa+2, directie);
  EEPROM.update(adresa+3, irpos);
  EEPROM.update(adresa+4, staysec);
  EEPROM.update(adresa+5, SPEED_MAX);
  EEPROM.update(adresa+6, SPEED_MIN);
  EEPROM.update(adresa+7, SPEED_DELTA);
  digitalWrite(LED_RED, HIGH);
  delay(1000);
  digitalWrite(LED_RED, LOW);
}

// language
int setlimba() 
{
    lcd.setCursor (0,0);
    lcd.print ("Language / Limba");
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setlimbatemp;
    }

    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setlimbatemp = setlimbatemp + knobval;
    if (setlimbatemp < 0) 
    {
      setlimbatemp = 1;
    }
    if (setlimbatemp > 1) 
    {
      setlimbatemp = 0;
    }
    
    lcd.print (setlimbatemp);
    if (setlimbatemp == 0)
    lcd.print(" - english "); 
    else
    lcd.print(" - romana  "); 
    setlimba();
}

// IR sensor activation (normal or reverse)
int setirpos() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Pozitie senzori:");
    else 
      lcd.print("Sensors position");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setirpostemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setirpostemp = setirpostemp + knobval;
    if (setirpostemp < 0) 
    {
      setirpostemp = 1;
    }
    if (setirpostemp > 1) 
    {
      setirpostemp = 0;
    }
    lcd.print (setirpostemp);
    if (setirpostemp == 0)
    {
      if (setlimbatemp == 1) 
       lcd.print(" - normala    ");
      else 
       lcd.print(" - usual      ");  
    }
    else
    {
       if (setlimbatemp == 1) 
       lcd.print(" - inversa    ");
      else 
       lcd.print(" - inverse    ");  
    }
    setirpos();
}

// Movement direction (normal or reverse)
int setdir() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Deplasare:");
    else 
      lcd.print("Direction ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setdirtemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setdirtemp = setdirtemp + knobval;
    if (setdirtemp < 0) 
    {
      setdirtemp = 1;
    }
    if (setdirtemp > 1) 
    {
      setdirtemp = 0;
    }
    lcd.print(setdirtemp);
    if (setdirtemp == 0)
    {
      if (setlimbatemp == 1) 
       lcd.print(" - normala    ");
      else 
       lcd.print(" - usual      ");  
    }
    else
    {
       if (setlimbatemp == 1) 
       lcd.print(" - inversa    ");
      else 
       lcd.print(" - inverse    ");  
    }
    setdir();
}

// time to stay in pause
int setstaysec() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Timp pauza:     ");
    else 
      lcd.print("Pause time:     ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setstaysectemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setstaysectemp = setstaysectemp + knobval;
    if (setstaysectemp < 1) 
    {
      setstaysectemp = 1;
    }
    if (setstaysectemp > 30) 
    {
      setstaysectemp = 30;
    }
    if (setstaysectemp < 10) lcd.print(" ");
    lcd.print (setstaysectemp);
    lcd.print("s");
    setstaysec();
}

// maximum speed (max.255)
int setspeedmax() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Viteza maxima:  ");
    else 
      lcd.print("Maximum speed:  ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setspeedmaxtemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setspeedmaxtemp = setspeedmaxtemp + knobval;
    if (setspeedmaxtemp < 200) 
    {
      setspeedmaxtemp = 200;
    }
    if (setspeedmaxtemp > 255) 
    {
      setspeedmaxtemp = 255;
    }
    if (setspeedmaxtemp < 100) lcd.print(" ");
    if (setspeedmaxtemp < 10) lcd.print(" ");
    lcd.print (setspeedmaxtemp);
    lcd.print("/255, min=200");
    setspeedmax();
}

// minimum speed (typ.105)
int setspeedmin() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Viteza minima:  ");
    else 
      lcd.print("Minimum speed:  ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setspeedmintemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setspeedmintemp = setspeedmintemp + knobval;
    if (setspeedmintemp < 25) 
    {
      setspeedmintemp = 25;
    }
    if (setspeedmintemp > 150) 
    {
      setspeedmintemp = 150;
    }
    if (setspeedmintemp < 100) lcd.print(" ");
    if (setspeedmintemp < 10) lcd.print(" ");
    lcd.print (setspeedmintemp);
    lcd.print("/255, typ=105");
    setspeedmin();
}

// delta speed (typ.5)
int setspeeddelta() 
{
    lcd.setCursor (0,0);
    if (setlimbatemp == 1) 
      lcd.print("Increment viteza");
    else 
      lcd.print("Delta speed:    ");  
    pushlength = pushlengthset;
    pushlength = getpushlength ();
    if (pushlength != pushlengthset) {
      return setspeeddeltatemp;
    }
    lcd.setCursor (0,1);
    knob.write(0);
    delay (50);
    knobval=knob.read();
    if (knobval < -1) { //bit of software de-bounce
      knobval = -1;
    }
    if (knobval > 1) {
      knobval = 1;
    }
    setspeeddeltatemp = setspeeddeltatemp + knobval;
    if (setspeeddeltatemp < 2) 
    {
      setspeeddeltatemp = 2;
    }
    if (setspeeddeltatemp > 20) 
    {
      setspeeddeltatemp = 20;
    }
    if (setspeeddeltatemp < 100) lcd.print(" ");
    if (setspeeddeltatemp < 10) lcd.print(" ");
    lcd.print (setspeeddeltatemp);
    lcd.print("/255, typ=5");
    setspeeddelta();
}
