#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal lcd(3,5,6,7,8,9);
// DS18S20 Temperature chip i/o
OneWire ds(10);  // on pin 10
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);

//const int ledPin = 11;
const int knobPin = 2;
const int relay1 = 11;
const float gain = 50./1024;
// these variables store the values for the knob and LED level
float knobValue;
int count = 0;
int last = 0;
int next = 0;
int delta;
float temp;
float low;
float high;
int state = 0;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(14,0);
  lcd.print("v1.0.4");
  lcd.setCursor(0, 0);
  lcd.print("Count=");
  lcd.setCursor(0, 1);
  lcd.print("Porog=");
  lcd.setCursor(0, 2);
  lcd.print("Temp=");
  //attachInterrupt(0, pulse, RISING);
  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, LOW);
  state = 0;  
}

void loop(void) {
  // read the value from the input
  knobValue = (float)analogRead(knobPin);
  // remap the values from 10 bit input to 8 bit output
  //fadeValue = map(knobValue, 0, 1023, 0 , 254);
  low = gain*knobValue;
  high = low + 1;
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  // use the input value to fade the led
  //analogWrite(ledPin, fadeValue);
  lcd.setCursor(6, 0);
  lcd.print(count);
  lcd.setCursor(6, 1);
  lcd.print(low); 
  lcd.setCursor(5, 2);
  lcd.print(temp);  
  
  if (state == 1) {
     if (temp < low) {
       digitalWrite(relay1, LOW);
       state = 0;    
     } 
  } else {
     if (temp > high) {
       digitalWrite(relay1, HIGH);    
       state = 1;
     } 
  }  
  pulse();
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

void pulse() {
  count++;
  next = millis();
  delta = (next - last);
  last = next;
}
  



