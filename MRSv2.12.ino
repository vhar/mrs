/*
 * Model Railroad Speedometer v2.12 2016.09.20 08:16 GMT
 * http://ctpahhuk.ru/node/69
 */

// include the libraries code:
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Bounce2.h>

// variables that can be changed
const int S = 98; //distance between center of left and right sensors in millimeters
const int resultDelay = 5000; // time display of measurement results in milliseconds
const int animationDelay = 50; // direction ticker delay time in milliseconds
const unsigned long timeoutDelay = 160000; // the waiting time of the second sensor signal in milliseconds
const int decimals = 2; // the accuracy of the result display

// arrays, which can added or changed
const String scale[][2] = {{" HO", "87"}, {" TT", "120"}, {"  N", "160"}}; // array of scales (display on-screen, the multiplier)
const String units[][2] = {{"KPH", "3.6"}, {"MPH", "2.23694"}}; // units array (display on-screen, conversion factor from meter per second)

// Creates a Bounce object, which you will later assign to an input pin.
Bounce scaleBouncer = Bounce();
Bounce unitsBouncer = Bounce();

LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // (RS, E, DB4, DB5, DB6, DB7) initialize the library with the numbers of the interface pins
const int leftPin = 9; // left sensor pin
const int rightPin = 8; // right sensor pin
const int timerPin = 8; // timer sensor pin
const int scalePin = 11; // scale/options button pin
const int unitsPin = 10; // units/mode pin

// calculations variables
unsigned long timeStart; // the time of receiving a signal from first sensor
unsigned long timeStop;// the time of receiving a signal from second sensor
unsigned long T; // interval in milliseconds
float V; // speed in meter per second
float sV; // scale speed
unsigned long currentLap =0; // time of current lap
unsigned long previousLap = 0; // time of previous lap
unsigned long penultLap = 0; // time of penultimate lap
unsigned long totlLaps = 0; // total passed laps
unsigned long totlTime = 0; // total time from timer start
String tmpStr; // template string for formatting output to LCD

// flags
unsigned long flagLeft = 0; // the time of receiving a signal from left sensor
unsigned long flagRight = 0; // the time of receiving a signal from right sensor
unsigned long flagTimer = 0; // the time of receiving a signal from timer sensor
unsigned long flagShow = 0; // end time of display results
unsigned long flagAnimation = 0; // the time of displaying next character in direction ticker
int flagRightLast = LOW; // previous state of right sensor
int flagRightCurrent = LOW; // current state of right sensor
int flagLeftLast = LOW; // previous state of left sensor
int flagLeftCurrent = LOW; // current state of left sensor
int flagTimerLast = LOW; // previous state of timer sensor
int flagTimerCurrent = LOW; // current state of timer sensor

int selected_scale = EEPROM.read(0); // stored key of scale array
int selected_unit = EEPROM.read(1); // stored key of units array
int selected_mode = 0; // selected mode (0 = speedometer, 1 = timer)
int scaleLast = LOW; // previous state of scale/options button
int scaleCurrent = LOW; // current state of scale/options button
int scaleUpdate = 0; // state of scale/options button changed
int unitsLast = LOW; // previous state of units/mode button
int unitsCurrent = LOW; // current state of units/mode button
int unitsUpdate = 0; // state of units/mode button changed
int screenShow = 0; // page of selected mode screen
unsigned long unitsHoldTime; // hold-on time of units/mode button for switch mode

// variables for direction ticker
int r;
char c;

void setup() {
  lcd.begin(16, 2);
  pinMode(leftPin, INPUT);
  pinMode(rightPin, INPUT);
  pinMode(scalePin, INPUT);
  pinMode(unitsPin, INPUT);
  // Bounce options
  scaleBouncer.attach(scalePin);
  scaleBouncer.interval(5);
  unitsBouncer.attach(unitsPin);
  unitsBouncer.interval(5);
}

// speed calculation
void calculateSpeed(int selected_unit, int selected_scale, int decimals){
  T = (timeStop - timeStart);
  V = S * 1000.0 / T;
  sV = V * units[selected_unit][1].toFloat() * scale[selected_scale][1].toFloat() * 0.001;
  char buffer[12];
  tmpStr = dtostrf(sV, 12, decimals, buffer);
  lcd.setCursor(0, 0);
  lcd.print(tmpStr);
  flagShow = millis() + resultDelay;
  timeStart = 0;
  timeStop = 0;
}

// ticker
void animation() {
  lcd.setCursor(r, 1);
  lcd.print(c);
  if(flagRight > 0) {
    r++;
    if(r > 11){
      r = 1;
      if(c == '>') {
        c = ' ';
      }
      else {
        c = '>';
      }
    }
  }
  else if(flagLeft > 0) {
    r--;
    if(r < 1){
      r = 11;
      if(c == '<') {
        c = ' ';
      }
      else {
        c = '<';
      }
    }
  }
  flagAnimation = millis() + animationDelay;
}

// print formatted time on LCD
void printTime(unsigned long elapsed){
  int h, m,s,ms;
  unsigned long over;
  h=int(elapsed/3600000);
  over=elapsed%3600000;
  m=int(over/60000);
  over=over%60000;
  s=int(over/1000);
  ms=over%1000;
  ms=int(ms/100);
  if(m < 10){
    lcd.print("0");
  }
  lcd.print(m);
  lcd.print(":");
  if(s < 10){
    lcd.print("0");
  }
  lcd.print(s);
  lcd.print(".");
  lcd.print(ms);
}

void loop() {
  // if the value read is greater than the number of elements in scale array - zero out the value
  if(selected_scale > sizeof(scale)/sizeof(scale[0])-1 || selected_scale < 0) {
    selected_scale = 0;
    EEPROM.update(0, 0);
  }
  // if the value read is greater than the number of elements in units array - zero out the value
  if(selected_unit > sizeof(units)/sizeof(units[0])-1 || selected_unit < 0) {
    selected_unit = 0;
    EEPROM.update(1, 0);
  }
  unitsUpdate = 0;
  scaleUpdate = 0;
  // check sensors state
  flagRightLast = flagRightCurrent;
  flagRightCurrent = digitalRead(rightPin);
  flagLeftLast = flagLeftCurrent;
  flagLeftCurrent = digitalRead(leftPin);
  flagTimerLast = flagTimerCurrent;
  flagTimerCurrent = digitalRead(timerPin);

  // state of the buttons has changed
  if(scaleBouncer.update()){
    scaleUpdate = 1;
    scaleLast = scaleCurrent;
    scaleCurrent = scaleBouncer.read();
  }
  if(unitsBouncer.update()){
    unitsUpdate = 1;
    unitsLast = unitsCurrent;
    unitsCurrent = unitsBouncer.read();
    if(unitsCurrent == LOW){
      unitsHoldTime = 0;
    }
    else {
      unitsHoldTime = millis();
    }
  }

  // if units/mode button is hold more than unitsHoldTime
  if (unitsHoldTime > 0 && millis() - unitsHoldTime > 1500) {
    unitsHoldTime = 0;
    if(selected_mode == 1){
      selected_mode = 0;
      screenShow = 0;
      lcd.clear();
      timeStart = 0;
      timeStop = 0;
      flagTimer = 0;
      currentLap = 0;
      penultLap = 0;
      previousLap = 0;
      totlLaps = 0;
      totlTime = 0;
    }
    else {
      lcd.clear();
      selected_mode = 1;
      if(selected_unit > 0){
        selected_unit--;
      }
      else {
       selected_unit = sizeof(units)/sizeof(units[0])-1;
      }
      screenShow = 0;
    }
  }
  // if display results flag less than the current time, running code below, otherwise continue to display the measurement results
  if(flagShow > millis()) {
    
  }
  // if selected mode = "Speedometer"
  else if(selected_mode == 0){
    flagShow = 0;
    // If receiving a signal from first sensor, but during the timeoutDelay time did not receiving a signal from the second sensor - display inscription "TIME OUT"
    if(timeStart > 0 && timeStart + timeoutDelay < millis()) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    TIME OUT    ");
      flagShow = millis() + resultDelay;
      timeStart = 0;
      timeStop = 0;
      flagLeft = 0;
      flagRight = 0;
      flagAnimation = 0;
    }
    // If simultaneously receiving a signal from both sensors worked, but did not set flags of the measurement start - display inscription "ERROR"
    else if(flagRightCurrent == LOW && flagLeftCurrent == LOW && flagLeft == 0 && flagRight == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("     ERROR      ");
      timeStart = 0;
      timeStop = 0;
      flagLeft = 0;
      flagRight = 0;
      flagAnimation = 0;
      flagShow = millis() + 1000;
    }
    // if there is no signal from both sensors and flags of both sensors not set - is ready for measurements
    else if(flagRightCurrent == HIGH && flagLeftCurrent == HIGH && flagLeft == 0 && flagRight == 0) {
      // if pressed scale/options button - change selected scale
      if(scaleUpdate == 1 && scaleCurrent == HIGH){
        selected_scale++;
        if(selected_scale > sizeof(scale)/sizeof(scale[0])-1){
          selected_scale = 0;
        }
      }
      // if pressed units/mode button - change selected units
      if(unitsUpdate == 1 && unitsCurrent == HIGH){
        selected_unit++;
        if(selected_unit > sizeof(units)/sizeof(units[0])-1){
          selected_unit = 0;
        }
      }
      timeStart = 0;
      timeStop = 0;
      flagLeft = 0;
      flagRight = 0;
      flagAnimation = 0;
      char buffer[12];
      tmpStr = dtostrf(0.000, 12, decimals, buffer);
      lcd.setCursor(0, 0);
      lcd.print(tmpStr);
      lcd.setCursor(13, 0);
      lcd.print(units[selected_unit][0]);
      lcd.setCursor(13, 1);
      lcd.print(scale[selected_scale][0]);
      lcd.setCursor(0, 1);
      lcd.print(" READY      ");
    }
    // if the measurement is not started and received signal from one of the sensors and flag for second sensor not set - start measuring
    else if(timeStart == 0 && ((flagRightCurrent == LOW && flagLeft == 0) || (flagLeftCurrent == LOW && flagRight == 0))) {
      timeStart = millis();
      if(flagRightCurrent == LOW){
        flagRight = millis();
        c = '>';
        r = 1;
      }
      else {
        flagLeft = millis();
        c = '<';
        r = 11;
      }
      lcd.setCursor(1, 1);
      lcd.print("           ");
      animation();
    }
    // if received signal from one of the sensors and the flag for second sensor is set - finish measurement and go to speed calculation
    else if((flagRightCurrent == LOW && flagLeft > 0) || (flagLeftCurrent == LOW && flagRight > 0)) {
      timeStop = millis();
      flagLeft = 0;
      flagRight = 0;
      flagAnimation = 0;
      EEPROM.update(0, selected_scale);
      EEPROM.update(1, selected_unit);
      lcd.setCursor(0, 1);
      lcd.print(" RESULT     ");
      calculateSpeed(selected_unit, selected_scale, decimals);
    }
    // if it is time to draw a new ticker symbol
    else if(flagAnimation < millis()){
      animation();
    }
  }
  // if selected mode = "Timer"
  else if(selected_mode == 1){
    flagShow = 0;
    // if pressed units/mode button
    if(unitsUpdate == 1 && unitsCurrent == HIGH){
      // if displayed second page - change it to first page and display timers
      if(screenShow == 1){
        screenShow = 0;
        lcd.setCursor(0, 0);
        printTime(previousLap);
        lcd.setCursor(0, 1);
        printTime(penultLap);
        lcd.setCursor(9,0);
        printTime(currentLap);
        lcd.setCursor(9,1);
        printTime(totlTime);
      }
      // else - change it to second page and display total passed laps and average laps time
      else {
        screenShow = 1;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LAPS");
        lcd.setCursor(0, 1);
        lcd.print("AVG");
        char buffer[7];
        tmpStr = dtostrf(totlLaps, 7, 0, buffer);
        lcd.setCursor(9, 0);
        lcd.print(tmpStr);
        T = 0;
        if(totlLaps > 0){
          T = (flagTimer - timeStop) / totlLaps;
        }
        lcd.setCursor(9,1);
        printTime(T);
      }
    }
    
    // if pressed scale/options button
    if(scaleUpdate == 1 && scaleCurrent == HIGH && screenShow == 0){
      if(timeStart  == 0 && flagTimer == 0){
        //if timer not started, show inscription "WAIT" and wait first time signal from timer sensor
        timeStart = millis();
        lcd.setCursor(11, 1);
        lcd.print(" WAIT");
      }
      else if(timeStart > 0){
        // else if timer started - stop it
        timeStart = 0;
      }
      else {
        // else - clear timers counter
        timeStart = 0;
        timeStop = 0;
        flagTimer = 0;
        currentLap = 0;
        penultLap = 0;
        previousLap = 0;
        totlLaps = 0;
        totlTime = 0;
      }
    }
    if(timeStart == 0 && flagTimer == 0 && screenShow == 0){
      // if timer not started and waiting and show first page - it is ready to start
      lcd.setCursor(0, 0);
      lcd.print("00:00.0  00:00.0");
      lcd.setCursor(0, 1);
      lcd.print("00:00.0    START");
    }
    else if (timeStart > 0 && flagTimer == 0 && flagTimerCurrent == LOW && flagTimerLast == HIGH) {
      // if waiting first signal from timer sensor and received signal from sensor - start timers
      flagTimer = millis();
      timeStop = flagTimer;
      if(screenShow == 0){
        lcd.setCursor(0, 0);
        printTime(previousLap);
        lcd.setCursor(0, 1);
        printTime(penultLap);
      }
    }
    else if (timeStart > 0 && flagTimer > 0 && flagTimerCurrent == LOW && flagTimerLast == HIGH) {
      // if timer works and received signal from sensor - calculate data
      timeStart = flagTimer;
      flagTimer = millis();
      penultLap = previousLap;
      previousLap = flagTimer - timeStart;
      totlLaps++;
      // if displayed first screen - show time of previous and penultimate laps
      if(screenShow == 0){
        lcd.setCursor(0, 0);
        printTime(previousLap);
        lcd.setCursor(0, 1);
        printTime(penultLap);
      }
      // if displayed second screen - show total passed laps and average laps time
      else {
        char buffer[7];
        tmpStr = dtostrf(totlLaps, 7, 0, buffer);
        lcd.setCursor(9, 0);
        lcd.print(tmpStr);
        T = 0;
        if(totlLaps > 0){
          T = (flagTimer - timeStop) / totlLaps;
        }
        lcd.setCursor(9,1);
        printTime(T);
      }
    }
    if(flagTimer > 0 && timeStart > 0){
      // if timer started and not receive signal from timer sensor - display current lap time and total time
      currentLap = millis() - flagTimer;
      totlTime = millis() - timeStop;
      if(screenShow == 0){
        lcd.setCursor(9,0);
        printTime(currentLap);
        lcd.setCursor(9,1);
        printTime(totlTime);
      }
    }
  }
  // if selected a non-existent mode - show inscription "ERROR"
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("     ERROR      ");
    timeStart = 0;
    timeStop = 0;
    flagLeft = 0;
    flagRight = 0;
    flagAnimation = 0;
    flagTimer = 0;
    currentLap = 0;
    penultLap = 0;
    previousLap = 0;
    totlLaps = 0;
    totlTime = 0;
    flagShow = millis() + 1000;
  }
}

