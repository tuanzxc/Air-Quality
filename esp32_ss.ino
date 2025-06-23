#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include "SdsDustSensor.h"
#include "rgb_lcd.h"
#include "time.h"
#include <ESP_Google_Sheet_Client.h>
// For SD/SD_MMC mounting helper
#include <GS_SDHelper.h>

#define BLYNK_TEMPLATE_ID "TMPL6M1wZtUfA"
#define BLYNK_TEMPLATE_NAME "air quality"
#define BLYNK_AUTH_TOKEN "_K3YxY00R9PfUUnqVQyx1q6c4KShTYCM"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define WIFI_SSID "GalaxyJ89C42"
#define WIFI_PASSWORD "nvt2124??"

// Google Project ID
#define PROJECT_ID "dust-461311"

// Service Account's client email
#define CLIENT_EMAIL "dust-ss@dust-461311.iam.gserviceaccount.com"

#define DHTPIN 2
#define DHTTYPE DHT11

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCwMEqUCEOcfd+W\ni8PMIm/y+oy/Do3zxsqqq1Spg6KGMf/U54L4Qj3KiKQdl/28EZsvvAq17/WTha/2\nStDaXANyEdxhZryRI+uzq9AIbm61IdpjDgeGsjMlmBUVr+ovHmDexJzER2DVaV1T\n1i6Kj57XaiReRLAOYMQ5sBzymlPbRkeu0fVSwFSXNySCNLeOVAUU+OxPXQy7R5x8\n8r2jlTUL6iFt/E4gg5coQU6Zg6w7VeuX2txEeCjoIosVwMDBHteV78s0VrsetFW4\nfw0SbDk8rB9FVj+ILMH0A6aU3K8Ghz97JaY8z3VyiGgWZ7SZDAw0pqO0nuFLORmy\n3uwMNRaVAgMBAAECggEAEO6t1tODKUCmQpdEsOiaWP/1PiNNnHZStzDnXlx/5mUF\nu4CoWJLdJ2AXDZ+0II/nRHYjkrVF+kXTJwNXq7SYwYIFQPhuT4Xj5WIyCFgR8acY\nOtl3+weYh24PO1oAUpjudwTu3/oldz0SCA2XGXU7WbkiAkvtnjl0gkAc+BUW0VrN\nfIjIVm+OwUCUj92CTizOAYYNHzOhfWgw5Al6zNHvWR0XBeiyQIAOAjQaiaIrfq3b\nsXJ+YbmRlyRx+wtfBvuHLxk6TOC0gdAtlWCZOUqmiaAw8x0h91sUXml/HZiPKvYk\nKPf5Li1RyyQRe0o2HPPbfzK6xnZ2/hJj1gQoZsO4PQKBgQD1aXpHHJkHKXPMCp0y\nEgmsOL61zP4ziXr9b5edFieZcqEhMWEzU1T2Irg7vqK+agYSD4uWOnhgxdQMYYNP\nrZ6buETu0qFvv2WYDrUw/bh0RtJ9i1+Faa5tAMPWOS4zUQDL12iE/8VB017rPcrg\nTtDejesfi8gga2D6mmD5rXe/BwKBgQC3ykGn4B/bRr4+zTdD+j8z7VY+6aJVMHwJ\nMiOHzrBT3ugtaF2Wc4Fc5wvvpymA3cMiATyK+pHBMgIJEbJ8GpdhrnmANOAzW1LU\neFC5heDhJMehIaXFaYz2QL0rudEOImATx3KGgf7ICn/VnYNCouTledLHu17NlpXO\nFfxZ87N6gwKBgQCdwfVR20df/DytELGma0P+iufFlIZgeOMwIdkzqPdBxBdicAzM\n4qe+JemKtmyFvUAzwfx4URPrGaDLaK/xu2k3LwhZAdu3KzFSwzmkyaUWUiSjgcGg\n0KPI/HHntxBE+rBpWQqeXJDgVNEypaFR+jh4qIBZ4hB/Dqfj0PO/7MwKZwKBgA2y\nUPpFxZn9x6e+e2PhflxMT6UYnDRlDJlS1zsDuDhH4F2h7XeM3BMkVMtjVcB4xlX+\naZih28LeCzDJ1NdS0/0/l2ZqilrJb37OCJYK2BtvNkFX1JbFqu+fG3nqux2miipv\nLqW/glE+FxDAUEvi/9PqJD8mQ4ZmtntJQCkj9DgpAoGAL0jFifthNiEBim+o6rpW\nKjj2QEojpKulWgWQBc9zV08GyyUHwlQ9q+0zsY+ftLwglmBcyonm6pVM/nUk0Rm2\nB0DG92PdIkLbYGp5P0Tc95nUIVWOmoeHft4OToLmg6W4nmG3pKTu3IhkfvNw0jo5\n0nMBP2pjxMSadJbx02F50+o=\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1uAiXf1gSndSc-ZgpiRZ3wS1UUuMsRgMqdL47ZvWy5EQ";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;

// Token Callback function
void tokenStatusCallback(TokenInfo info);

DHT dht(DHTPIN, DHTTYPE);

// Variable to save current epoch time
unsigned long epochTime; 

// Function that gets current epoch time
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }

  // Chuyển struct tm -> time_t
  time_t rawTime = mktime(&timeinfo);
  
  // Cộng thêm 7 giờ (tính bằng giây)
  rawTime += 7 * 3600;

  // Chuyển lại thành struct tm sau khi cộng
  struct tm* adjustedTime = localtime(&rawTime);

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", adjustedTime);
  return String(buffer);
}


HardwareSerial mySerial(2); // UART1
int rxPin = 16; // GPIO3 (U1RX)
int txPin = 17; // GPIO2 (U1TX)
SdsDustSensor sds(mySerial, rxPin, txPin);
int pm25 = 0, pm10 = 0, aqi_pm25 = 0, aqi_pm10 = 0, aqi = 0;
rgb_lcd lcd;
int calculateAQI(float concentration, float cLow, float cHigh, int iLow, int iHigh) {
  return round((iHigh - iLow) * (concentration - cLow) / (cHigh - cLow) + iLow);
}

int calcAQI_PM25(float pm25) {
  if (pm25 <= 25) return calculateAQI(pm25, 0, 25, 0, 50);
  else if (pm25 <= 50) return calculateAQI(pm25, 26, 50, 51, 100);
  else if (pm25 <= 80) return calculateAQI(pm25, 51, 80, 101, 150);
  else if (pm25 <= 150) return calculateAQI(pm25, 81, 150, 151, 200);
  else if (pm25 <= 250) return calculateAQI(pm25, 151, 250, 201, 300);
  else if (pm25 <= 350) return calculateAQI(pm25, 251, 350, 301, 400);
  else if (pm25 <= 500) return calculateAQI(pm25, 351, 500, 401, 500);
  else return 500;
}

int calcAQI_PM10(float pm10) {
  if (pm10 <= 50) return calculateAQI(pm10, 0, 50, 0, 50);
  else if (pm10 <= 150) return calculateAQI(pm10, 51, 150, 51, 100);
  else if (pm10 <= 250) return calculateAQI(pm10, 151, 250, 101, 150);
  else if (pm10 <= 350) return calculateAQI(pm10, 251, 350, 151, 200);
  else if (pm10 <= 420) return calculateAQI(pm10, 351, 420, 201, 300);
  else if (pm10 <= 500) return calculateAQI(pm10, 421, 500, 301, 400);
  else if (pm10 <= 600) return calculateAQI(pm10, 501, 600, 401, 500);
  else return 500;
}

void setup(){
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    dht.begin();
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    //Configure time
    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    // Connect to Wi-Fi
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
    mySerial.begin(9600, SERIAL_8N1, rxPin, txPin);
    lcd.begin(16, 2);
    lcd.clear();
}

void loop(){
    // Call ready() repeatedly in loop for authentication checking and processing
    bool ready = GSheet.ready();
    Blynk.run();
    if (ready && millis() - lastTime > timerDelay){
        lastTime = millis();

        FirebaseJson response;

        Serial.println("\nAppend spreadsheet values...");
        Serial.println("----------------------------");

        FirebaseJson valueRange;
        PmResult pm = sds.readPm();
  // Đọc giá trị nhiệt độ và độ ẩm
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();
      if (pm.isOk()) {
        pm25 = pm.pm25;
        pm10 = pm.pm10;
        aqi_pm25 = calcAQI_PM25(pm25);
        aqi_pm10 = calcAQI_PM10(pm10);
        aqi = max(aqi_pm25, aqi_pm10);
        lcd.setCursor(0, 0); lcd.print("PM25:");
        lcd.setCursor(5, 0); lcd.print(pm25);
        lcd.setCursor(0, 1); lcd.print("PM10:");
        lcd.setCursor(5, 1); lcd.print(pm10);
      }
  // Kiểm tra lỗi trong quá trình đọc
        if (isnan(humidity) || isnan(temperature)) {
          Serial.println("Lỗi đọc cảm biến!");
        }
      lcd.setCursor(9, 0); lcd.print("T:");
      lcd.setCursor(11, 0); lcd.print(temperature);
      lcd.setCursor(15, 0); lcd.print("C");
      lcd.setCursor(9, 1); lcd.print("AQI:");
      lcd.setCursor(13, 1); lcd.print(aqi);
  // In giá trị lên Serial Monitor
      //Serial.print("Độ ẩm: ");
      //Serial.print(humidity);
      //Serial.print(" %\t");
      //Serial.print("Nhiệt độ: ");
      //Serial.print(temperature);
      //Serial.println(" °C");        // New BME280 sensor readings
      Blynk.virtualWrite(V0, pm25);
      Blynk.virtualWrite(V1, pm10);
      Blynk.virtualWrite(V4, aqi);
      Blynk.virtualWrite(V2, temperature);
      Blynk.virtualWrite(V3, humidity);
        // Get timestamp
        String formattedTime = getFormattedTime();

        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", formattedTime);
        valueRange.set("values/[1]/[0]", pm25);
        valueRange.set("values/[2]/[0]", pm10);
        valueRange.set("values/[3]/[0]", temperature);
        valueRange.set("values/[4]/[0]", humidity);        
        valueRange.set("values/[5]/[0]", aqi);        

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
        // Append values to the spreadsheet
        bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
        if (success){
            response.toString(Serial, true);
            valueRange.clear();
        }
        else{
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
        Serial.println(ESP.getFreeHeap());
    }
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}