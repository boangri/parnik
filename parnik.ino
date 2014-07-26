#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char version[] = "1.1.5";

const int knob1Pin = A2;
const int knob2Pin = A3;
const int humiditySensorPin = A4;
//const int distanceSensorPin = A4;
const int dividerPin = A5;
const int tempPin = 10;
const int fanPin = 11;
const int pumpPin = 12;

const int N = 4;

LiquidCrystal lcd(3,5,6,7,8,9);
// DS18S20 Temperature chip i/o
OneWire ds(tempPin);  // on pin 10
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);

const float gain = 50./1023;
// these variables store the values for the knob and LED level
float knob1Value;
float knob2Value;
float dividerValue;
float humValue;
//int count = 0;
//int last = 0;
//int next = 0;
//int delta;
float temp;
float temp_avg = 0.0;
float humidity;
float humidity_avg = 0.0;
float volt;
float volt_avg = 0.0;
float temp_lo, temp_hi;
float hum_lo, hum_hi;
int fanState = 0;
int pumpState = 0;
int i = 0; // iteration counter;
float navg ; // number of samples for averaging
float workHours, fanHours, pumpHours; // work time (hours)
unsigned long fanMillis, pumpMillis, workMillis, delta;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("v=");
  lcd.setCursor(2,0);
  lcd.print(version);
  
  lcd.setCursor(0, 1);
  lcd.print("U=");
  
  lcd.setCursor(0, 2);
  lcd.print("T=");
  lcd.setCursor(7, 2);
  lcd.print("/");
  
  lcd.setCursor(0, 3);
  lcd.print("H=");
  lcd.setCursor(7, 3);
  lcd.print("/");
  
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(fanPin, LOW);
  digitalWrite(pumpPin, LOW);
  fanState = 0; 
  fanMillis = 0;
  pumpState = 0;
  pumpMillis = 0; 
  Serial.println(version);
  workMillis = millis();
}

void loop(void) {
  // for averaging
  float p,q;
  i++;
  if (i < N) {
    navg = (float)i;
  } else {
    navg = (float)N;
  }   
  q = 1.0/navg;
  p = 1.0 - q;
  // Timing
  delta = millis() - workMillis;
  workMillis += delta;
  if (fanState) {
    fanMillis += delta;
  }
  if (pumpState) {
    pumpMillis += delta;
  }
  workHours = workMillis/3600000.; // Hours;
  fanHours = fanMillis/3600000.;
  pumpHours = pumpMillis/3600000.;
  
  // read the value from the input
  knob1Value = (float)analogRead(knob1Pin);
  knob2Value = (float)analogRead(knob2Pin);
  knob2Value = map(knob2Value, 0, 1023, 0 , 100);
  humValue = (float)analogRead(humiditySensorPin);
  //humValue = map(humValue, 0, 1023, 0 , 100);
  dividerValue = (float)analogRead(dividerPin);
  // remap the values from 10 bit input to 8 bit output
  //fadeValue = map(knobValue, 0, 1023, 0 , 254);
  temp_lo = gain*knob1Value;
  temp_hi = temp_lo + 1.0;
  hum_lo = knob2Value;
  hum_hi = hum_lo + 2.;
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  humidity = 1.0 * humValue;
  humidity_avg = p*humidity_avg +q*humidity;
  volt = 12.77/2.55*3./1023.* dividerValue;
  volt_avg = p*volt_avg + q*volt;
  
  lcd.setCursor(14, 0);
  lcd.print(workHours);
  
  lcd.setCursor(2, 1);
  lcd.print(volt_avg); 
  
  lcd.setCursor(2, 2);
  lcd.print(temp);  
  lcd.setCursor(8, 2);
  lcd.print(temp_lo);
  lcd.setCursor(14, 2);
  lcd.print(fanHours);
  
  lcd.setCursor(2, 3);
  lcd.print(humidity);
  lcd.setCursor(8, 3);
  lcd.print(hum_lo);
  lcd.setCursor(14, 3);
  lcd.print(pumpHours);
  
 /* fan control */ 
  if (fanState == 1) {
     if (temp < temp_lo) {
       digitalWrite(fanPin, LOW);
       fanState = 0;    
     } 
  } else {
     if (temp > temp_hi) {
       digitalWrite(fanPin, HIGH);    
       fanState = 1;
     } 
  }  
  /* pump control */
  if (pumpState == 1) {
     if (humidity_avg > hum_hi) {
       digitalWrite(pumpPin, LOW);
       pumpState = 0;    
     } 
  } else {
     if (humidity_avg < hum_lo) {
       digitalWrite(pumpPin, HIGH);    
       pumpState = 1;
     } 
  }  
  delay(1000);
}  
  



