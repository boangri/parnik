#include <Average.h>
#include <dht.h>
#include <CRC16.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>

CRC16 crc16(1);
DHT dht = DHT();
Average voltage(N_AVG);
Average temperature(N_AVG);
Average distance(N_AVG);
#include "RS485.h"

const char version[] = "2.3.0"; /* Averages */

#define TEMP_FAN 26  // temperature for fans switching off
#define TEMP_PUMP 20 // temperature - do not pump water if cold enought
#define BARREL_HEIGHT 74.0 // max distanse from sonar to water surface which 
#define BARREL_DIAMETER 57.0 // 200L

#define ON LOW
#define OFF HIGH // if transistors 

const int tempPin = A0;
const int echoPin = A1;
const int knob1Pin = A2;
const int knob2Pin = A3;
const int dhtPin = A4;
const int dividerPin = A5;
const int triggerPin = 10;
const int fanPin = 11;
const int pumpPin = 12;

const int N = 100;

const float Vpoliv = 1.0; // Liters per centigrade above TEMP_PUMP 
const float Tpoliv = 4; // Watering every 4 hours

LiquidCrystal lcd(3,5,6,7,8,9);
// DS18S20 Temperature chip i/o
OneWire ds(tempPin);  // on pin 10
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&ds);
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar(triggerPin, echoPin, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

const float gain = 50./1023;
// these variables store the values for the knob and LED level
float knob1Value;
float knob2Value;
float dividerValue;
float humValue;
float distValue;
float temp;
float temp_avg = 0.0;
float humidity;
float humidity_avg = 0.0;
float volt;
float volt_avg = 0.0;
float sig, sig_avg = 0.0;
float water, water0;
float temp_lo, temp_hi;
float hum_lo, hum_hi;
int fanState = 0;
int pumpState = 0;
int it = 0; // iteration counter;
float navg ; // number of samples for averaging
float workHours, fanHours, pumpHours; // work time (hours)
unsigned long fanMillis, pumpMillis, workMillis, delta, lastTemp, lastDist, ms;
int np = 0; /* poliv session number */
float h; // distance from sonar to water surface, cm.
int nmeas;
float volt_sum;
float volt2_sum;
float barrel_height;
float barrel_volume;

Parnik parnik;
Parnik *pp = &parnik;

void setup(void) {
  Serial.begin(9600);
  sensors.begin();
  dht.attach(dhtPin);

  lcd_setup();
  
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(fanPin, OFF);
  digitalWrite(pumpPin, OFF);
  fanState = 0; 
  fanMillis = 0;
  pumpState = 0;
  pumpMillis = 0; 
  Serial.println(version);
  workMillis = 0; // millis();
  lastTemp = lastDist = millis();
  h = 200.;
  it = 0;
  np = 0;
  nmeas = 0;
  volt_sum = 0.;
  volt2_sum = 0.;
  RS485_setup(pp);
  barrel_height = BARREL_HEIGHT;
  barrel_volume = BARREL_DIAMETER*BARREL_DIAMETER*3.14/4000*BARREL_HEIGHT;
}

void loop(void) {
  unsigned int uS;
  
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
  // measure water volume
  uS = sonar.ping_median(7);
  if (uS > 0) {
    h = (float)uS / US_ROUNDTRIP_CM;
  }  
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
  dht.update();
  if(dht.getLastError() == DHT_ERROR_OK) {
     pp->temp2 = dht.getTemperatureInt();
     pp->hum1 = dht.getHumidityInt();
  }  
  // read the value from the input

  dividerValue = (float)analogRead(dividerPin);
  pp->temp_lo = temp_lo = TEMP_FAN;;
  pp->temp_hi = temp_hi = temp_lo + 1.0;
  
  //if ((millis() - lastTemp > 30000) || (lastTemp == 0)){ /* measure temperature only once in 30 sec */
    sensors.requestTemperatures();
    temp = sensors.getTempCByIndex(0);
    temperature.putValue(temp);
    
    pp->temp1 = temp;
    lastTemp = millis();
  //}
  
  volt = 55.52/1023.* dividerValue;
  nmeas++;
  volt_sum += volt;
  volt2_sum += volt*volt;
  if (nmeas == 100) {
    float fn = (float)nmeas;
    pp->volt = volt_sum/fn;
    pp->sigma = sqrt(volt2_sum/fn - pp->volt*pp->volt); 
    nmeas = 0;
    volt_sum = 0.;
    volt2_sum = 0.; 
  }
  
  serial_output();
  lcd_output();  
  
 /* fan control */ 
  if (fanState == 1) {
     if (temp < temp_lo) {
       digitalWrite(fanPin, OFF);
       fanState = 0;  
       pp->fans = fanState;  
     } 
  } else {
     if (temp > temp_hi) {
       digitalWrite(fanPin, ON);    
       fanState = 1;
       pp->fans = fanState;
     } 
  }  
  /* pump control */
  if (pumpState == 1) {
    float V;
    V = (temp - TEMP_PUMP) * Vpoliv;
    //V = np == 1 ? 0.5 : V;  // First poliv - just for test
     if ((water < 0.) || (temp < TEMP_PUMP) || (water0 - water > V)) {
       digitalWrite(pumpPin, OFF);
       pumpState = 0; 
       pp->pump = pumpState;   
     } 
  } else {
     if (workHours >= Tpoliv*np) {       
       np++;  
       // Switch on the pump only if warm enought and there is water in the barrel     
       if ((temp > TEMP_PUMP) && (water > 0.)) {
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
  return barrel_volume*(1.0 - h/barrel_height);
}  

void serial_output() {
  Serial.print(" it=");
  Serial.print(it);
  Serial.print(" H=");
  Serial.print(pp->hum1);
  Serial.print("% T=");
  Serial.print(pp->temp2);
  Serial.print(" sigV=");
  Serial.print(pp->sigma);
  Serial.print(" U=");
  Serial.print(volt);
  Serial.print(" Uavg=");
  Serial.print(pp->volt);
  Serial.print(" H: ");
  Serial.print(h);
  Serial.print(" cm. Volume: ");
  Serial.print(water);
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
  
  lcd.setCursor(0, 2);
  lcd.print("T=");
  lcd.setCursor(7, 2);
  lcd.print("/");
  
  lcd.setCursor(0, 3);
  lcd.print("H=");
  lcd.setCursor(7, 3);
  lcd.print("/");  
}

void lcd_output() {
  lcd.setCursor(14, 0);
  lcd.print(workHours);
  
  lcd.setCursor(2, 1);
  lcd.print(volt); 
  lcd.setCursor(10, 1);
  lcd.print(water); 
  
  lcd.setCursor(2, 2);
  lcd.print(temp);  
  lcd.setCursor(8, 2);
  lcd.print(temp_lo);
  lcd.setCursor(14, 2);
  lcd.print(fanHours);
  
  lcd.setCursor(2, 3);
  lcd.print(pp->hum1);
  lcd.setCursor(8, 3);
  lcd.print(hum_lo);
  lcd.setCursor(14, 3);
  lcd.print(pumpHours);  
}  
