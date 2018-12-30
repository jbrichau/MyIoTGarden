#include "PowerShield.h"

#define SOILPOWER A4 // Soil Sensor power
#define SOILSENSE A1 // Soil Sensor input

#define PUMP D4 // Pump power control

#define INTERVAL 60*30
#define DRY_PERCENTAGE_TRESHOLD 50

#define STATE_START 1
#define STATE_MEASURING 2
#define STATE_PUMPING 3
#define STATE_SENDING 4
#define STATE_WAITING 5

#define MILLISECONDSONLINE  1000*60*5

PowerShield batteryMonitor;
int sensorValue = 0;
int soilHumidity = 0;
const int dryValue = 3120;
const int wetValue = 1530;
double soc = 0;
int wifi = 0;
int measureCounter;
int pumpCounter;
int state;
unsigned long startTime;

// Set external antenna (remembered after power off)
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup() {
  startTime = millis();
  batteryMonitor.begin();
  batteryMonitor.quickStart();
  pinMode(SOILPOWER, OUTPUT);
  pinMode(PUMP,OUTPUT);
  digitalWrite(PUMP,LOW);
  Particle.variable("battery", soc);
  Particle.variable("soilhumidity", soilHumidity);
  measureCounter = 0;
  pumpCounter = 0;
  sensorValue = 0;
  state = STATE_START;
}

void loop() {
  switch (state) {
    case STATE_START:
      start_soil_measuring();
      break;

    case STATE_MEASURING:
      if(measureCounter < 3)
        measure_soil_iter();
      else {
        end_measuring();
        if(soilHumidity < DRY_PERCENTAGE_TRESHOLD)
          start_pumping();
        else
          start_sending();
      }
      break;

    case STATE_PUMPING:
      if(pumpCounter >= 3) {
        end_pumping();
        start_sending();
      } else pumping_iter();
      break;

    case STATE_SENDING:
      measure_wifi();
      measure_soc();
      Particle.publish("soildata", soil_data(), PRIVATE);
      state = STATE_WAITING;
      break;

    case STATE_WAITING:
      if(startTime + MILLISECONDSONLINE < millis())
        System.sleep(SLEEP_MODE_DEEP,INTERVAL);
      else
        delay(3000);
  }
}

String soil_data() {
  return String::format("{\"soilhumidity\": %d, \"soc\":%f,\"wifi\":%d,\"pumped\": %d}",soilHumidity,soc,wifi);
}

void start_soil_measuring() {
  state = STATE_MEASURING;
  digitalWrite(SOILPOWER, HIGH);
  delay(3000);
}

void measure_soil_iter() {
  sensorValue += analogRead(SOILSENSE);
  measureCounter++;
  delay(3000);
}

void end_measuring() {
  sensorValue = sensorValue / measureCounter;
  soilHumidity = map(sensorValue, wetValue, dryValue, 100, 0);
}

void start_pumping() {
  state = STATE_PUMPING;
  digitalWrite(PUMP,HIGH);
}

void pumping_iter() {
  pumpCounter++;
  delay(5000);
}

void end_pumping() {
  digitalWrite(PUMP,LOW);
}

void start_sending() {
  digitalWrite(SOILPOWER, LOW);
  state = STATE_SENDING;
}

void measure_wifi() {
  int wifi_db = WiFi.RSSI();
  if(wifi_db < 0)
    wifi = map(wifi_db, -1, -127, 100, 0);
}

void measure_soc() {
  soc = batteryMonitor.getSoC();
}