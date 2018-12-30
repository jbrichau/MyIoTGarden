#include "PowerShield.h"

#define SOILPOWER A4 // Soil Sensor power
#define SOILSENSE A1 // Soil Sensor input

#define PUMP D4 // Pump power control

#define INTERVAL 60
#define DRY_PERCENTAGE_TRESHOLD 50

PowerShield batteryMonitor;
int sensorValue = 0;
int percentage = 0;
const int dryValue = 3120;
const int wetValue = 1530;
double soc = 0;
int wifi;
int counter;

// Set external antenna (remembered after power off)
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup() {
  batteryMonitor.begin();
  batteryMonitor.quickStart();
  pinMode(SOILPOWER, OUTPUT);
  pinMode(PUMP,OUTPUT);
  digitalWrite(PUMP,LOW);
  Particle.variable("sensorValue", sensorValue);  
  Particle.variable("battery", soc);
  Particle.variable("percentage", percentage);
  counter = 0;
  sensorValue = 0;
}

void loop() {
  measure_wifi();
  measure_soc();
  digitalWrite(SOILPOWER, HIGH);
  delay(1000);
  sensorValue += analogRead(SOILSENSE);
  digitalWrite(SOILPOWER, LOW);
  delay(5000);
  counter++;
  if(counter>=3) {
    sensorValue = sensorValue / counter;
    percentage = map(sensorValue, wetValue, dryValue, 100, 0);
    if(percentage<DRY_PERCENTAGE_TRESHOLD)
      pumpwater();
    Particle.publish("soildata", soil_data(), PRIVATE);
    System.sleep(SLEEP_MODE_DEEP,INTERVAL);
  }
}

String soil_data() {
  return String::format("{\"sensorValue\":%d, \"percentage\": %d, \"soc\":%f,\"wifi\":%d}",sensorValue,percentage,soc,wifi);
}

void measure_wifi() {
  int wifi_db = WiFi.RSSI();
  if(wifi_db < 0)
    wifi = map(wifi_db, -1, -127, 100, 0);
}

void measure_soc() {
  soc = batteryMonitor.getSoC();
}

void pumpwater() {
  digitalWrite(PUMP,HIGH);
  delay(10000);
  digitalWrite(PUMP,LOW);
}
