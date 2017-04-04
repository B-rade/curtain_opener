#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

// clock
RTC_DS3231 rtc;

// button pins
const int hrUpPin = 0;
const int hrDownPin = 3;
const int minUpPin = 4;
const int minDownPin = 5;
const int openCurtainPin = 6;
const int closeCurtainPin = 7;
const int okPin = 8;
const int curtainPin = 9;
const int lightSensorPin = A0;

// motors
const int motPwm = 10;
const int motIn1 = 12;
const int motIn2 = 11;
const int motStandby = 13; //Pull high to enable driver

// states
int hrUpState = 0;
int hrDownState = 0;
int minUpState = 0;
int minDownState = 0;
int openCurtainState = 0;
int closeCurtainState = 0;
int okState = 0;
boolean curtainState = 0; //1 closed, 0 open
int dispOn = 1;
int lightLevel = 0;

// Timing
int delayTime = 500;
int openTime = 0;

// alarm time set by default to 10:00 AM
DateTime alarm (delayTime, 1, 1, 10, 0, 0);
DateTime currentTime;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.init();
  lcd.backlight();

  // set up the button pins
  pinMode(hrUpPin, INPUT_PULLUP);
  pinMode(hrDownPin, INPUT_PULLUP);
  pinMode(minUpPin, INPUT_PULLUP);
  pinMode(minDownPin, INPUT_PULLUP);
  pinMode(openCurtainPin, INPUT_PULLUP);
  pinMode(closeCurtainPin, INPUT_PULLUP);
  pinMode(okPin, INPUT_PULLUP);
  pinMode(lightSensorPin, INPUT);

  pinMode(motPwm, OUTPUT);
  pinMode(motIn1, OUTPUT);
  pinMode(motIn2, OUTPUT);
  pinMode(motStandby, OUTPUT);

  lcd.setCursor(0, 1);
  lcd.print(" Open Time: 10:00");
  lcd.setCursor(0, 2);
  lcd.print(" Light Level:");
  lcd.setCursor(0, 3);

  currentTime = rtc.now();
  updateTime(currentTime);
}

void loop() {

  // this executes every 10 seconds
  if ((currentTime + TimeSpan(0, 0, 0, 5)).unixtime() < rtc.now().unixtime()) {
    currentTime = rtc.now();
    updateTime(currentTime);

    // Poll the light-level
    lightPoll();
  }

  // Poll the buttons to see if any are pressed
  buttonPoll();

  if (curtainState == 1 && (currentTime.hour() == alarm.hour())) {
    openCurtain();
  } else if (curtainState == 0 && lightLevel <= 150) {
    closeCurtain();
  }

  // Make decisions based on what needs to be done



}

void lightPoll() {
  lightLevel = analogRead(lightSensorPin);
  lcd.setCursor(14, 2);
  lcd.print("     ");
  lcd.setCursor(14, 2);
  lcd.print(lightLevel);
}

void buttonPoll() {
  // hrUp
  hrUpState = digitalRead(hrUpPin);
  if (hrUpState == LOW) {
    alarm = alarm + TimeSpan(0, 1, 0, 0);
    clearAlarmTime();
    udpateAlarmTime(alarm);
    delay(delayTime);
  }

  // hrDown
  hrDownState = digitalRead(hrDownPin);
  if (hrDownState == LOW) {
    alarm = alarm - TimeSpan(0, 1, 0, 0);
    clearAlarmTime();
    udpateAlarmTime(alarm);
    delay(delayTime);
  }

  // minUp
  minUpState = digitalRead(minUpPin);
  if (minUpState == LOW) {
    alarm = alarm + TimeSpan(0, 0, 1, 0);
    clearAlarmTime();
    udpateAlarmTime(alarm);
    delay(delayTime);
  }

  // minDown
  minDownState = digitalRead(minDownPin);
  if (minDownState == LOW) {
    alarm = alarm - TimeSpan(0, 0, 1, 0);
    clearAlarmTime();
    udpateAlarmTime(alarm);
    delay(delayTime);
  }

  // openCurtain (spins the motor CCW)
  openCurtainState = digitalRead(openCurtainPin);
  curtainState = digitalRead(curtainPin);
  while (openCurtainState == LOW && curtainState == HIGH) {
    openCurtain();
    openCurtainState = digitalRead(openCurtainPin);
    curtainState = digitalRead(curtainPin);
  }

  // closeCurtain (spins the motor CW)
  closeCurtainState = digitalRead(closeCurtainPin);
  curtainState = digitalRead(curtainPin);
  while (closeCurtainState == LOW && curtainState == HIGH) {
    closeCurtain();
    closeCurtainState = digitalRead(closeCurtainPin);
    curtainState = digitalRead(curtainPin);
  }

  // OK button
  okState = digitalRead(okPin);
  if (okState == LOW) {
    if (dispOn) {
      lcd.noBacklight();
      dispOn = 0;
      delay(delayTime);
    } else {
      lcd.backlight();
      dispOn = 1;
      delay(delayTime);
    }
  }
}

void clearAlarmTime() {
  lcd.setCursor(12, 1);
  lcd.print("     ");
}

void clearLightLevel() {
  lcd.setCursor(14, 2);
  lcd.print("     ");
}

//TODO can format this nicer
// HH:MM (AM/PM) MM/DD/YYYY
void updateTime(DateTime t) {
  lcd.setCursor(0, 0);
  lcd.print("                    ");
  lcd.setCursor(0, 0);
  lcd.print(t.hour(), DEC);
  lcd.print(":");

  // add leading 0's to times less than 10 minutes
  if (t.minute() / 10 < 1) {
    lcd.print("0");
  }
  lcd.print(t.minute(), DEC);

  lcd.print("   ");
  lcd.print(t.month(), DEC);
  lcd.print("/");
  lcd.print(t.day(), DEC);
  lcd.print("/");
  lcd.print(t.year(), DEC);
}

void udpateAlarmTime(DateTime alarm) {
  // keep alignment of colon the same by switching the position of the cursor
  if (alarm.hour() / 10 > 0) {
    lcd.setCursor(12, 1);
  } else {
    lcd.setCursor(13, 1);
  }

  lcd.print(alarm.hour(), DEC);
  lcd.print(":");

  // add leading 0's to times less than 10 minutes
  if (alarm.minute() / 10 < 1) {
    lcd.print("0");
  }
  lcd.print(alarm.minute(), DEC);
}

void closeCurtain() {
  digitalWrite(motStandby, 1);
  digitalWrite(motIn1, 0);
  digitalWrite(motIn2, 1);
  int startTime = millis();
  do {
    analogWrite(motPwm, 191);
  } while ((millis() - startTime) < openTime);
  digitalWrite(motStandby, 0);
  curtainState = digitalRead(curtainPin);
}

void openCurtain() {
  digitalWrite(motStandby, 1);
  digitalWrite(motIn1, 1);
  digitalWrite(motIn2, 0);
  int startTime = millis();
  do {
    analogWrite(motPwm, 191);
  } while (digitalRead(curtainPin) == 1);
  int endTime = millis();
  openTime = endTime - startTime;
  digitalWrite(motStandby, 0);
  curtainState = digitalRead(curtainPin);
}


