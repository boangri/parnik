#include <Average.h>
#include <dht.h>
#include <CRC16.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>

CRC16 crc16(1);

Average voltage(N_AVG);
//Average temperature1(N_AVG);
//Average temperature2(N_AVG);
Average distance(N_AVG);
#include "RS485.h"

const char version[] = "2.5.3"; /* LCD */

#define TEMP_FANS 27  // temperature for fans switching on
#define TEMP_PUMP 20 // temperature - do not water if cold enought
#define BARREL_HEIGHT 76.0 // max distanse from sonar to water surface which 
#define BARREL_DIAMETER 57.0 // 200L

#define ON LOW
#define OFF HIGH // if transistors 

const int tempPin = A0;
const int echoPin = A1;
//const int dhtPin = A2;
const int dividerPin = A5;
const int triggerPin = 10;
const int fanPin = 11;
const int pumpPin = 12;

const float Vpoliv = 1.0; // Liters per centigrade above TEMP_PUMP 
const float Tpoliv = 3; // Watering every 3 hours
//DHT dht = DHT();
LiquidCrystal lcd(3,5,6,7,8,9);
// DS18S20 Temperature chip i/o
OneWire ds(tempPin);  // on pin 10
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);
int numberOfDevices; 
boolean convInProgress;

#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar(triggerPin, echoPin, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

float dividerValue;
float humValue;
float distValue;
//float temp;
float temp_avg = 0.0;
float humidity;
float humidity_avg = 0.0;
float volt;
float water, water0;
int fanState = 0;
int pumpState = 0;
int it = 0; // iteration counter;
float workHours, fanHours, pumpHours; // work time (hours)
unsigned long fanMillis, pumpMillis, workMillis, delta, lastTemp, lastDist, lastVolt;
int np = 0; /* poliv session number */
float h; // distance from sonar to water surface, cm.
float barrel_height;
float barrel_volume;

Parnik parnik;
Parnik *pp = &parnik;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  convInProgress = false;
  numberOfDevices = sensors.getDeviceCount();
  Serial.print("Locating devices...");
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
//  dht.attach(dhtPin);

  lcd_setup();
  
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(fanPin, OFF);
  digitalWrite(pumpPin, OFF);
  fanState = 0; 
  fanMillis = 0;
  pumpState = 0;
  pumpMillis = 0; 
  pp->temp_hi = TEMP_FANS;
  pp->temp_lo = pp->temp_hi - 1.0;
  pp->temp_pump = TEMP_PUMP;
  Serial.println(version);
  workMillis = 0; // millis();
  lastTemp = lastDist = lastVolt = millis();
  h = 200.;
  it = 0;
  np = 0;
  RS485_setup(pp);
  barrel_height = BARREL_HEIGHT;
  barrel_volume = BARREL_DIAMETER*BARREL_DIAMETER*3.14/4000*BARREL_HEIGHT;
}

void loop(void) {
  unsigned int uS;
  unsigned int ms;
  
  if(Serial1.available() > 0) RS485(pp);
  
  it++;
  // Timing
  delta = millis() - workMillis;
  workMillis += delta;
  if (fanState) {
    fanMillis += delta;
  }
  if (pumpState) {
    pumpMillis += delta;
  }
  workHours = (float)workMillis/3600000.; // Hours;
  fanHours = (float)fanMillis/3600000.;
  pumpHours = (float)pumpMillis/3600000.;

  /* 
   *  Measure temperature
   */
  if(!convInProgress) { //launch temp converson
    sensors.setWaitForConversion(false);  // makes it async
    sensors.requestTemperatures();
    sensors.setWaitForConversion(true);
    lastTemp = millis();
    convInProgress = true;   
  }
  if(millis() - lastTemp > 750) { // data should be ready
    pp->temp1 = sensors.getTempCByIndex(0);
    //temperature1.putValue(temp); 
    if (numberOfDevices > 1) {
      pp->temp2 = sensors.getTempCByIndex(1);
      //temperature2.putValue(temp);
    }
    convInProgress = false;
    //temp = pp->temp1;
  }
  
  /* 
   *  measure water volume
   */
  uS = sonar.ping_median(7);
  if (uS > 0) {
    h = (float)uS / US_ROUNDTRIP_CM;
  }  
  h *= (1 + 0.5*(pp->temp1 - 20)/(pp->temp1 + 273)); // take temperature into account
  distance.putValue(h);
  ms = millis();
  if (ms - lastDist > 3000) {
    pp->dist = distance.getAverage();
    water = toVolume(pp->dist);
    pp->vol = water;
    lastDist = ms;
  }
  
  //////
  //
  // DHT-11 sensor - temperature and humidity
  //
  /*
  dht.update();
  if(dht.getLastError() == DHT_ERROR_OK) {
     pp->temp2 = dht.getTemperatureInt();
     pp->hum1 = dht.getHumidityInt();
  }  
  */
  
  /* 
   *  Measure voltage
   */
  dividerValue = (float)analogRead(dividerPin);  
  volt = 55.52/1023.* dividerValue;
  voltage.putValue(volt);
  ms = millis();
//  if (ms - lastVolt > 3000) {
//    pp->volt = voltage.getAverage();
//    lastVolt = ms;
//  }
  /*
   * Output data
   */
  if (Serial.available() > 0) { 
    Serial.read();
    serial_output();
  }   
  lcd_output();  
  
  /*
   * Fans control 
   */ 
  if (fanState == 1) {
     if (pp->temp1 < pp->temp_lo) {
       digitalWrite(fanPin, OFF);
       fanState = 0;  
       pp->fans = fanState;  
     } 
  } else {
     if (pp->temp1 > pp->temp_hi) {
       digitalWrite(fanPin, ON);    
       fanState = 1;
       pp->fans = fanState;
     } 
  }  
  /*
   * Pump control 
   */
  if (pumpState == 1) {
    float V;
    V = (pp->temp1 - pp->temp_pump) * Vpoliv;
    //V = np == 1 ? 0.5 : V;  // First poliv - just for test
     if ((water < 0.) || (pp->temp1 < pp->temp_pump) || (pp->temp2 < pp->temp_pump) || (water0 - water > V)) {
       digitalWrite(pumpPin, OFF);
       pumpState = 0; 
       pp->pump = pumpState;   
     } 
  } else {
     if (workHours >= Tpoliv*np) {       
       np++;  
       // Switch on the pump only if warm enought and there is water in the barrel     
       if ((pp->temp1 > pp->temp_pump) && (pp->temp2 > pp->temp_pump) && (water > 0.)) {
         digitalWrite(pumpPin, ON);    
         pumpState = 1;
         pp->pump = pumpState;
         water0 = water;
       }
     }  
  }  
}  

/*
 * Given a distance to water surface (in sm.)
 * calculates the volume of water in the barrel (in Liters)
 */
float toVolume(float h) {
  //float vol;
  return barrel_volume*(1.0 - h/barrel_height);
  //vol -= 0.5*(barrel_volume - vol)*(pp->temp1 - 20)/(pp->temp1 + 273);
  //return vol;
}  

void serial_output() {
  Serial.print(" it=");
  Serial.print(it);
  /*
  Serial.print(" H=");
  Serial.print(pp->hum1);
  Serial.print("% T=");
  Serial.print(pp->temp2);
  */
  Serial.print(" U=");
  Serial.print(volt);
  Serial.print(" Uavg=");
  Serial.print(pp->volt);
  Serial.print(" H: ");
  Serial.print(h);
  Serial.print(" cm. Volume: ");
  Serial.print(pp->vol);
  Serial.println(" L.");
}

void lcd_setup() {
  lcd.begin(20, 4);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("v=");
  lcd.setCursor(2,0);
  lcd.print(version);
  
  lcd.setCursor(0, 1);
  lcd.print("U=");
  lcd.setCursor(8, 1);
  lcd.print("W=");
 /* 
  lcd.setCursor(0, 2);
  lcd.print("T=");
  lcd.setCursor(7, 2);
  lcd.print("/");
  
  lcd.setCursor(0, 3);
  lcd.print("H=");
  lcd.setCursor(7, 3);
  lcd.print("/");  
  */
}

void lcd_output() {
  char buf[10];
  lcd.setCursor(10, 0);
  millis2str(buf, workMillis);
  lcd.print(buf);
  
  lcd.setCursor(2, 1);
  lcd.print(volt); 
  lcd.setCursor(10, 1);
  if(water < 100.) lcd.print(' ');
  if(water < 10.) lcd.print(' ');
  lcd.print(water); 
  
  lcd.setCursor(0, 2);
  lcd.print(pp->temp1);
  lcd.setCursor(4, 2);
  lcd.print("/");  
  lcd.setCursor(5, 2);
  lcd.print(pp->temp_hi);
  lcd.setCursor(10, 2);
  millis2str(buf, fanMillis);
  lcd.print(buf);
  
  lcd.setCursor(0, 3);
  lcd.print(pp->temp2);
  lcd.setCursor(4, 3);
  lcd.print("/");
  lcd.setCursor(5, 3);
  lcd.print(pp->temp_pump);
  lcd.setCursor(10, 3);
  millis2str(buf, pumpMillis);
  lcd.print(buf); 
}  

void millis2str(char buf[], unsigned long ms) {
  int hours, mins, secs;
  ms /= 1000;
  secs = ms % 60;
  ms -= secs;
  ms /= 60;
  mins = (ms % 60);
  ms -= mins;
  hours = ms /60;  
  sprintf(buf, "%4d:%02d:%02d", hours, mins, secs);
}

