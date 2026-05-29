#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "SdsDustSensor.h"
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
#include <GS_SDHelper.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include "esp_wifi.h"
#include "esp_sleep.h"

// =====================================================
// WIFI RESET BUTTON
// =====================================================

#define RESET_WIFI_PIN 15
#define HOLD_TIME 3000

unsigned long pressStart = 0;
bool pressed = false;

// =====================================================
// GOOGLE SHEET
// =====================================================

#define PROJECT_ID "dust-461311"
#define CLIENT_EMAIL "dust-ss@dust-461311.iam.gserviceaccount.com"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCwMEqUCEOcfd+W\ni8PMIm/y+oy/Do3zxsqqq1Spg6KGMf/U54L4Qj3KiKQdl/28EZsvvAq17/WTha/2\nStDaXANyEdxhZryRI+uzq9AIbm61IdpjDgeGsjMlmBUVr+ovHmDexJzER2DVaV1T\n1i6Kj57XaiReRLAOYMQ5sBzymlPbRkeu0fVSwFSXNySCNLeOVAUU+OxPXQy7R5x8\n8r2jlTUL6iFt/E4gg5coQU6Zg6w7VeuX2txEeCjoIosVwMDBHteV78s0VrsetFW4\nfw0SbDk8rB9FVj+ILMH0A6aU3K8Ghz97JaY8z3VyiGgWZ7SZDAw0pqO0nuFLORmy\n3uwMNRaVAgMBAAECggEAEO6t1tODKUCmQpdEsOiaWP/1PiNNnHZStzDnXlx/5mUF\nu4CoWJLdJ2AXDZ+0II/nRHYjkrVF+kXTJwNXq7SYwYIFQPhuT4Xj5WIyCFgR8acY\nOtl3+weYh24PO1oAUpjudwTu3/oldz0SCA2XGXU7WbkiAkvtnjl0gkAc+BUW0VrN\nfIjIVm+OwUCUj92CTizOAYYNHzOhfWgw5Al6zNHvWR0XBeiyQIAOAjQaiaIrfq3b\nsXJ+YbmRlyRx+wtfBvuHLxk6TOC0gdAtlWCZOUqmiaAw8x0h91sUXml/HZiPKvYk\nKPf5Li1RyyQRe0o2HPPbfzK6xnZ2/hJj1gQoZsO4PQKBgQD1aXpHHJkHKXPMCp0y\nEgmsOL61zP4ziXr9b5edFieZcqEhMWEzU1T2Irg7vqK+agYSD4uWOnhgxdQMYYNP\nrZ6buETu0qFvv2WYDrUw/bh0RtJ9i1+Faa5tAMPWOS4zUQDL12iE/8VB017rPcrg\nTtDejesfi8gga2D6mmD5rXe/BwKBgQC3ykGn4B/bRr4+zTdD+j8z7VY+6aJVMHwJ\nMiOHzrBT3ugtaF2Wc4Fc5wvvpymA3cMiATyK+pHBMgIJEbJ8GpdhrnmANOAzW1LU\neFC5heDhJMehIaXFaYz2QL0rudEOImATx3KGgf7ICn/VnYNCouTledLHu17NlpXO\nFfxZ87N6gwKBgQCdwfVR20df/DytELGma0P+iufFlIZgeOMwIdkzqPdBxBdicAzM\n4qe+JemKtmyFvUAzwfx4URPrGaDLaK/xu2k3LwhZAdu3KzFSwzmkyaUWUiSjgcGg\n0KPI/HHntxBE+rBpWQqeXJDgVNEypaFR+jh4qIBZ4hB/Dqfj0PO/7MwKZwKBgA2y\nUPpFxZn9x6e+e2PhflxMT6UYnDRlDJlS1zsDuDhH4F2h7XeM3BMkVMtjVcB4xlX+\naZih28LeCzDJ1NdS0/0/l2ZqilrJb37OCJYK2BtvNkFX1JbFqu+fG3nqux2miipv\nLqW/glE+FxDAUEvi/9PqJD8mQ4ZmtntJQCkj9DgpAoGAL0jFifthNiEBim+o6rpW\nKjj2QEojpKulWgWQBc9zV08GyyUHwlQ9q+0zsY+ftLwglmBcyonm6pVM/nUk0Rm2\nB0DG92PdIkLbYGp5P0Tc95nUIVWOmoeHft4OToLmg6W4nmG3pKTu3IhkfvNw0jo5\n0nMBP2pjxMSadJbx02F50+o=\n-----END PRIVATE KEY-----\n";
const char spreadsheetId[] = "1uAiXf1gSndSc-ZgpiRZ3wS1UUuMsRgMqdL47ZvWy5EQ";

// =====================================================
// NTP
// =====================================================
const char* ntpServer = "pool.ntp.org";

// =====================================================
// SDS011
// =====================================================

HardwareSerial mySerial(2);

#define RX_PIN 16
#define TX_PIN 17

SdsDustSensor sds(mySerial, RX_PIN, TX_PIN);

// =====================================================
// DHT11
// =====================================================

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================================
// BATTERY
// =====================================================

#define BAT_PIN 34

float batteryVoltage = 0;
int batteryLevel = 0;

// =====================================================
// SD
// =====================================================

#define SD_CS 5

// =====================================================
// DEEP SLEEP
// =====================================================

const uint64_t sleepTimeUs = 60ULL * 1000000ULL;

// =====================================================
// DATA
// =====================================================

int pm25 = 0;
int pm10 = 0;
int aqi = 0;

float temperature = 0;
float humidity = 0;

// =====================================================
// TIME
// =====================================================

String getFormattedTime(){

  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){

    return "1970-01-01 00:00:00";
  }

  time_t rawTime = mktime(&timeinfo) + 7 * 3600;

  struct tm* adjusted = localtime(&rawTime);

  char buffer[30];

  strftime(
    buffer,
    sizeof(buffer),
    "%Y-%m-%d %H:%M:%S",
    adjusted
  );

  return String(buffer);
}

// =====================================================
// AQI
// =====================================================

int calculateAQI(
  float c,
  float cLow,
  float cHigh,
  int iLow,
  int iHigh){

  return round(
    (iHigh - iLow) *
    (c - cLow) /
    (cHigh - cLow) +
    iLow
  );
}

int calcAQI_PM25(float pm25){

  if(pm25 <= 25)
    return calculateAQI(pm25,0,25,0,50);

  else if(pm25 <= 50)
    return calculateAQI(pm25,26,50,51,100);

  else if(pm25 <= 80)
    return calculateAQI(pm25,51,80,101,150);

  else if(pm25 <= 150)
    return calculateAQI(pm25,81,150,151,200);

  else if(pm25 <= 250)
    return calculateAQI(pm25,151,250,201,300);

  else if(pm25 <= 350)
    return calculateAQI(pm25,251,350,301,400);

  return calculateAQI(pm25,351,500,401,500);
}

int calcAQI_PM10(float pm10){

  if(pm10 <= 50)
    return calculateAQI(pm10,0,50,0,50);

  else if(pm10 <= 150)
    return calculateAQI(pm10,51,150,51,100);

  else if(pm10 <= 250)
    return calculateAQI(pm10,151,250,101,150);

  else if(pm10 <= 350)
    return calculateAQI(pm10,251,350,151,200);

  else if(pm10 <= 420)
    return calculateAQI(pm10,351,420,201,300);

  else if(pm10 <= 500)
    return calculateAQI(pm10,421,500,301,400);

  return calculateAQI(pm10,501,600,401,500);
}

// =====================================================
// SAVE SD
// =====================================================

void saveToSD(String data){

  File file = SD.open("/unsent.csv", FILE_APPEND);

  if(file){

    file.println(data);

    file.close();

    Serial.println("Backup OK");
  }
  else{

    Serial.println("Backup FAIL");
  }
}

// =====================================================
// RESEND DATA
// =====================================================

void resendUnsentData(){

  if(!SD.exists("/unsent.csv"))
    return;

  File file = SD.open("/unsent.csv");

  if(!file){

    Serial.println("Open unsent fail");

    return;
  }

  int sent = 0;
  int total = 0;

  while(file.available() && sent < 10){

    String line = file.readStringUntil('\n');

    line.trim();

    if(line.isEmpty())
      continue;

    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1);
    int c4 = line.indexOf(',', c3 + 1);
    int c5 = line.indexOf(',', c4 + 1);
    int c6 = line.indexOf(',', c5 + 1);

    if(c6 == -1)
      continue;

    FirebaseJson valueRange, response;

    valueRange.add("majorDimension","COLUMNS");

    valueRange.set("values/[0]/[0]", line.substring(0,c1));
    valueRange.set("values/[1]/[0]", line.substring(c1+1,c2));
    valueRange.set("values/[2]/[0]", line.substring(c2+1,c3));
    valueRange.set("values/[3]/[0]", line.substring(c3+1,c4));
    valueRange.set("values/[4]/[0]", line.substring(c4+1,c5));
    valueRange.set("values/[5]/[0]", line.substring(c5+1,c6));
    valueRange.set("values/[6]/[0]", line.substring(c6+1));

    if(GSheet.values.append(
      &response,
      spreadsheetId,
      "Sheet2!A1",
      &valueRange)){

      Serial.println("Resend OK");

      sent++;
      total++;
    }
    else{

      Serial.println("Resend FAIL");

      break;
    }
  }

  file.close();

  if(total > 0){

    File orig = SD.open("/unsent.csv");

    File temp = SD.open("/temp.csv", FILE_WRITE);

    int skipped = 0;
    int remain = 0;

    while(orig.available()){

      String l = orig.readStringUntil('\n');

      if(skipped < total){

        skipped++;

        continue;
      }

      l.trim();

      if(!l.isEmpty()){

        temp.println(l);

        remain++;
      }
    }

    orig.close();

    temp.close();

    SD.remove("/unsent.csv");

    if(remain > 0){

      SD.rename("/temp.csv","/unsent.csv");
    }
    else{

      SD.remove("/temp.csv");
    }
  }
}

// =====================================================
// TOKEN CALLBACK
// =====================================================

void tokenStatusCallback(TokenInfo info){

  if(info.status == token_status_error){

    Serial.println(GSheet.getTokenError(info));
  }
}

// =====================================================
// BUTTON
// =====================================================

void checkButton(){
  if(digitalRead(RESET_WIFI_PIN) == LOW){
    Serial.println("CONFIG MODE");
    WiFiManager wm;
    wm.startConfigPortal("ESP32_Config");
  }
}

// =====================================================
// BATTERY
// =====================================================

int getBatteryLevel(float vbat){

  if(vbat >= 4.1) return 10;
  if(vbat >= 4.0) return 9;
  if(vbat >= 3.9) return 8;
  if(vbat >= 3.8) return 7;
  if(vbat >= 3.7) return 6;
  if(vbat >= 3.6) return 5;
  if(vbat >= 3.5) return 4;
  if(vbat >= 3.4) return 3;
  if(vbat >= 3.3) return 2;
  if(vbat >= 3.2) return 1;

  return 0;
}

// =====================================================
// LIGHT SLEEP
// =====================================================

void enterDeepSleep(){

  Serial.println("Deep Sleep");

  // Sleep SDS011
  sds.sleep();

  delay(100);

  // Tắt WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  esp_wifi_stop();

  delay(100);

  Serial.flush();

  // Wakeup timer
  esp_sleep_enable_timer_wakeup(sleepTimeUs);

  // Wakeup button
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);

  // Deep sleep
  esp_deep_sleep_start();
}

// =====================================================
// SETUP
// =====================================================

void setup(){

  Serial.begin(115200);

  Serial.println("System Start");

  analogReadResolution(9);
  analogSetAttenuation(ADC_11db);

  pinMode(RESET_WIFI_PIN, INPUT_PULLUP);
  checkButton();

  // =================================================
  // SD
  // =================================================
  if(!SD.begin(SD_CS)){
    Serial.println("SD FAIL");
  }
  else{
    Serial.println("SD OK");
  }

  // =================================================
  // DHT
  // =================================================

  dht.begin();

  // =================================================
  // SDS
  // =================================================

  mySerial.begin(
    9600,
    SERIAL_8N1,
    RX_PIN,
    TX_PIN
  );

  sds.wakeup();
  Serial.println("SDS Wakeup");
  // =================================================
  // WIFI
  // =================================================
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("Connecting WiFi");
  unsigned long start = millis();
  while(WiFi.status() != WL_CONNECTED &&
        millis() - start < 15000){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("WiFi Connected");
    WiFi.setSleep(true);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    // =================================================
    // TIME
    // =================================================
    configTime(0,0,ntpServer);
    
    // =================================================
    // GOOGLE SHEET
    // =================================================
    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(600);
    GSheet.begin(
      CLIENT_EMAIL,
      PROJECT_ID,
      PRIVATE_KEY
    );
    Serial.println("Waiting Google Auth");
    unsigned long authStart = millis();
    while(!GSheet.ready() && millis() - authStart < 15000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();
    if(GSheet.ready()){
      Serial.println("Google Ready");
    }
    else{
      Serial.println("Google Auth Timeout");
    }
    delay(20000);
  }
  else{
    Serial.println("WiFi Connect FAIL");
    delay(15000);
  }

  // =================================================
  // RESEND
  // =================================================
  resendUnsentData();
  Serial.println("System Ready");
}

// =====================================================
// LOOP
// =====================================================
void loop(){
  Serial.println("===== NEW READ =====");

  // =================================================
  // SDS READ
  // =================================================
  PmResult pm = sds.readPm();

  // =================================================
  // DHT
  // =================================================

  humidity = dht.readHumidity();

  temperature = dht.readTemperature();

  if(isnan(humidity))
    humidity = 0;

  if(isnan(temperature))
    temperature = 0;

  // =================================================
  // PM
  // =================================================

  if(pm.isOk()){

    pm25 = pm.pm25;

    pm10 = pm.pm10;

    int aqi25 = calcAQI_PM25(pm25);

    int aqi10 = calcAQI_PM10(pm10);

    aqi = max(aqi25, aqi10);
  }

  // =================================================
  // BATTERY
  // =================================================

  analogRead(BAT_PIN);

  delay(10);

  long sum = 0;

  for(int i = 0; i < 30; i++){

    sum += analogRead(BAT_PIN);

    delay(2);
  }

  float raw = sum / 30.0;

  float vadc = raw * 3.3 / 511.0;

  batteryVoltage = vadc * 2.0 * 1.04;

  if(batteryVoltage > 4.2)
    batteryVoltage = 4.2;

  batteryLevel = getBatteryLevel(batteryVoltage);

  Serial.printf(
    "Battery: %.2fV  Level: %d/10\n",
    batteryVoltage,
    batteryLevel
  );

  // =================================================
  // CSV
  // =================================================

  String time = getFormattedTime();

  String csvLine =
    time + "," +
    String(pm25) + "," +
    String(pm10) + "," +
    String(aqi) + "," +
    String(temperature) + "," +
    String(humidity) + "," +
    String(batteryLevel);

  Serial.println(csvLine);

  // =================================================
  // SEND
  // =================================================

  FirebaseJson valueRange, response;

  valueRange.add("majorDimension","COLUMNS");

  valueRange.set("values/[0]/[0]", time);
  valueRange.set("values/[1]/[0]", pm25);
  valueRange.set("values/[2]/[0]", pm10);
  valueRange.set("values/[3]/[0]", aqi);
  valueRange.set("values/[4]/[0]", temperature);
  valueRange.set("values/[5]/[0]", humidity);
  valueRange.set("values/[6]/[0]", batteryLevel);

  bool sendOK = false;

  if(WiFi.status() == WL_CONNECTED &&
     GSheet.ready()){

    sendOK = GSheet.values.append(
      &response,
      spreadsheetId,
      "Sheet2!A1",
      &valueRange
    );
  }
  // =================================================
  // RESULT
  // =================================================

  if(sendOK){
    Serial.println("Send OK");
  }
  else{

    Serial.println("Send FAIL -> backup");

    saveToSD(csvLine);
  }

  // =================================================
  // LIGHT SLEEP
  // =================================================
  enterDeepSleep();}