#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char version[] = "1.1.2";

const int knob1Pin = A2;
const int knob2Pin = A3;
const int humiditySensorPin = A4;
const int dividerPin = A5;
const int tempPin = 10;
const int fanPin = 11;
const int pumpPin = 12;

LiquidCrystal lcd(3,5,6,7,8,9);
// DS18S20 Temperature chip i/o
OneWire ds(tempPin);  // on pin 10
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);

const float gain = 50./1024;
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
float humidity;
float volt;
float temp_lo, temp_hi;
float hum_lo, hum_hi;
int fanState = 0;
int pumpState = 0;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(14,0);
  lcd.print(version);
  lcd.setCursor(0, 0);
  lcd.print("Humid=");
  lcd.setCursor(0, 1);
  lcd.print("Porog=");
  lcd.setCursor(0, 2);
  lcd.print("Temp=");
  lcd.setCursor(0, 3);
  lcd.print("Humidity=");
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(fanPin, LOW);
  digitalWrite(pumpPin, LOW);
  fanState = 0; 
  pumpState = 0; 
  Serial.println(version);
}

void loop(void) {
  // read the value from the input
  knob1Value = (float)analogRead(knob1Pin);
  knob2Value = (float)analogRead(knob2Pin);
  humValue = (float)analogRead(humiditySensorPin);
  dividerValue = (float)analogRead(dividerPin);
  // remap the values from 10 bit input to 8 bit output
  //fadeValue = map(knobValue, 0, 1023, 0 , 254);
  temp_lo = gain*knob1Value;
  temp_hi = temp_lo + 1.0;
  hum_lo = knob2Value;
  hum_hi = hum_lo + 100.;
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  humidity = 1.0 * humValue;
  volt = 12.77/2.55*3./1023.* dividerValue;
  
  lcd.setCursor(6, 0);
  lcd.print(hum_lo);
  lcd.setCursor(6, 1);
  lcd.print(temp_lo); 
  lcd.setCursor(5, 2);
  lcd.print(temp);  
  lcd.setCursor(14, 2);
  lcd.print(volt);
  lcd.setCursor(9, 3);
  lcd.print(humidity);
  
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
     if (humidity > hum_hi) {
       digitalWrite(pumpPin, LOW);
       pumpState = 0;    
     } 
  } else {
     if (humidity < hum_lo) {
       digitalWrite(pumpPin, HIGH);    
       pumpState = 1;
     } 
  }  
  
  //pulse();
  /*
  digitalWrite(ledPin, HIGH);   // set the LED on
  delay(200);              // wait for a second
  digitalWrite(ledPin, LOW);    // set the LED off
  delay(800);              // wait for a second
  */
  
  //Serial.print("Requesting temperatures...");
  //sensors.requestTemperatures(); // Send the command to get temperatures
  //Serial.println("DONE");
  //temp = sensors.getTempCByIndex(0);
  //Serial.print("Temperature for the device 1 (index 0) is: ");
  //Serial.println(temp);  
}
/*
void pulse() {
  count++;
  next = millis();
  delta = (next - last);
  last = next;
}
*/  



