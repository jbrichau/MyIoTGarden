#include "PowerShield.h"

#define SOILPOWER A4 // Soil Sensor power
#define SOILSENSE A1 // Soil Sensor input

#define PUMP D4 // Pump power control

#define FLOWPOWER D6
#define FLOWSENSE D5 // Pin must support interrupt

#define INTERVAL 60*30

PowerShield batteryMonitor;
int sensorValue = 0;
int percentage = 0;
const int dryValue = 3120;
const int wetValue = 1530;
double soc = 0;
int wifi;
int counter;
int pulsed;

// Set external antenna (remembered after power off)
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup() {
  batteryMonitor.begin();
  batteryMonitor.quickStart();
  pinMode(SOILPOWER, OUTPUT);
  pinMode(FLOWPOWER, OUTPUT);
  pinMode(FLOWSENSE, INPUT_PULLUP);
  pinMode(PUMP,OUTPUT);
  digitalWrite(FLOWPOWER,LOW);
  digitalWrite(PUMP,LOW);
  Particle.variable("sensorValue", sensorValue);
  counter = 0;
  sensorValue = 0;
  pulsed = 0;
  attachInterrupt(FLOWSENSE, pulse, RISING);
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
    //if(percentage<50)
    //  pumpwater();
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

void pulse() {
  pulsed++;
}

void pumpwater() {
  int previous_pulsed;
  digitalWrite(FLOWPOWER,HIGH);
  digitalWrite(PUMP,HIGH);
  do {
    previous_pulsed = pulsed;
    delay(10000);
    Serial.print(pulsed);
  } while(pulsed > previous_pulsed+100 && pulsed <= 100000);
  digitalWrite(PUMP,LOW);
  digitalWrite(FLOWPOWER,LOW);
}
