#include "PowerShield.h"

#define INTERVAL 60*30  // Run every 30 minutes
#define DRY_PERCENTAGE_TRESHOLD 50  // Dry is less than 50% of the sensorvalue
#define MILLISECONDSONLINE  1000*60*5

#define SOILPOWER A4    // Soil Sensor power
#define SOILSENSE A1    // Soil Sensor input
#define PUMP D4         // Pump power control

#define STATE_START 1
#define STATE_MEASURING 2
#define STATE_PUMPING 3
#define STATE_CHECK_RESULT 4
#define STATE_SENDING 5
#define STATE_WAITING 6

PowerShield batteryMonitor;
const int dryValue = 3120;
const int wetValue = 1530;
int sensorValue = 0;
int soilHumidity = 0;
double soc = 0;
int wifi = 0;
int measureCounter = 0;
int pumpCounter = 0;
int outofwater = 0;
int state = STATE_START;
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
  digitalWrite(SOILPOWER, HIGH);
  Particle.variable("battery", soc);
  Particle.variable("soilhumidity", soilHumidity);
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
        start_checking();
      } else pumping_iter();
      break;

    case STATE_CHECK_RESULT:
      if(measureCounter < 3)
        measure_soil_iter();
      else {
        int previous_soilHumidity = soilHumidity;
        end_measuring();
        if(soilHumidity <= previous_soilHumidity)
          outofwater = 1;
        start_sending();
      }
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
  return String::format("{\"soilhumidity\": %d, \"soc\":%f,\"wifi\":%d,\"pumped\": %d,\"outofwater\": %d}",soilHumidity,soc,wifi,pumpCounter,outofwater);
}

void start_soil_measuring() {
  state = STATE_MEASURING;
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
  // TEMPORARILY DISABLE PUMP
  //digitalWrite(PUMP,HIGH);
}

void pumping_iter() {
  pumpCounter++;
  delay(5000);
}

void end_pumping() {
  digitalWrite(PUMP,LOW);
}

void start_checking() {
  state = STATE_CHECK_RESULT;
  sensorValue = 0;
  measureCounter = 0;
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