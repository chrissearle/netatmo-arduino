#include <Arduino.h>

#include <SPI.h>
#include <U8g2lib.h>

#include <ESP8266WiFi.h>
#include <HttpClient.h>

#include <ArduinoJson.h>

#define CLK_PIN D5
#define DIN_PIN D7
#define CE_PIN D1
#define DC_PIN D6
#define RST_PIN D2
#define LIGHT_PIN D0
#define SENSOR_PIN D4

const char* ssid = "SSID";
const char* password = "WIFIPASSWORD";

const char *kHostname = "HOST";
const char *kPath = "/RUTER_PATH.json";

U8G2_PCD8544_84X48_1_4W_SW_SPI u8g2(U8G2_R0, CLK_PIN, DIN_PIN, CE_PIN, DC_PIN, RST_PIN);

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;

const unsigned long pollIntervalFetchMillis = 60L * 1000;
const unsigned long pollIntervalScreenMillis = 5L * 1000;

unsigned long lastUpdateFetchMillis = -pollIntervalFetchMillis;
unsigned long lastUpdateScreenMillis = -pollIntervalScreenMillis;

int screenToShow = 0;
const int maxScreen = 2;

struct Rain {
  float rain;
  float rainDay;
  float rainHour;
};

struct Wind {
  float gust;
  float maxWind;
  float strength;
};

struct Indoor {
  float pressure;
  float humidity;
  float noise;
  float co2;
  float temperature;
  char pressureTrend[20];
  float minTemp;
  float maxTemp;
};

struct Outdoor {
  float temperature;
  char temperatureTrend[10];
  float humidity;
  float minTemp;
  float maxTemp;
};

struct Weather {
  struct Rain rain;
  struct Wind wind;
  struct Indoor indoor;
  struct Outdoor outdoor;
};

Weather weather;

#define Thermometer_width 10
#define Thermometer_height 36
static unsigned char Thermometer_bits[] = {
  0x30, 0x00, 0x48, 0x00, 0x84, 0x00, 0x84, 0x00, 0x84, 0x00, 0x84, 0x00, 
  0x84, 0x00, 0x84, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 
  0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 
  0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 
  0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB4, 0x00, 0x7A, 0x01, 0xFD, 0x02, 
  0xFD, 0x02, 0xFD, 0x02, 0xFD, 0x02, 0x7A, 0x01, 0x84, 0x00, 0x78, 0x00, 
  };
  
#define TrendUp_width 10
#define TrendUp_height 10
static unsigned char TrendUp_bits[] = {
  0xE0, 0x03, 0x00, 0x03, 0x80, 0x02, 0x40, 0x02, 0x20, 0x02, 0x10, 0x00, 
  0x08, 0x00, 0x04, 0x00, 0x02, 0x00, 0x01, 0x00, };

#define TrendDown_width 10
#define TrendDown_height 10
static unsigned char TrendDown_bits[] = {
  0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x02, 
  0x40, 0x02, 0x80, 0x02, 0x00, 0x03, 0xE0, 0x03, };

#define Wind_width 10
#define Wind_height 12
static unsigned char Wind_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0xCC, 0x00, 0x12, 0x01, 0x08, 0x01, 0xFF, 0x00, 
  0x00, 0x00, 0xFC, 0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 
  };

#define Rain_width 10
#define Rain_height 12
static unsigned char Rain_bits[] = {
  0x40, 0x00, 0x40, 0x00, 0xA0, 0x00, 0xA4, 0x00, 0x14, 0x01, 0x1A, 0x01, 
  0xEA, 0x00, 0x51, 0x01, 0x51, 0x01, 0x2E, 0x02, 0x20, 0x02, 0xC0, 0x01, 
  };

void setup(void) {
  Serial.begin(115200);

  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println();
  Serial.println("Connected");
  
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  memset(&weather, '\0', sizeof(Weather));

  u8g2.begin();
}

void extractWeather(struct Weather *weather, JsonObject *data) {
  JsonObject& root = *data;

  weather->rain.rain = root["Rain"]["Rain"];
  weather->rain.rainDay = root["Rain"]["RainDay"];
  weather->rain.rainHour = root["Rain"]["RainHour"];

  weather->wind.gust = root["Wind"]["GustStrength"];
  weather->wind.maxWind = root["Wind"]["MaxWind"];
  weather->wind.strength = root["Wind"]["WindStrength"];

  weather->indoor.pressure = root["Indoor"]["Pressure"];
  weather->indoor.humidity = root["Indoor"]["Humidity"];
  weather->indoor.noise = root["Indoor"]["Noise"];
  weather->indoor.co2 = root["Indoor"]["CO2"];
  weather->indoor.temperature = root["Indoor"]["Temperature"];
  weather->indoor.minTemp = root["Indoor"]["MinTemp"];
  weather->indoor.maxTemp = root["Indoor"]["MaxTemp"];
  strcpy(weather->indoor.pressureTrend, root["Indoor"]["PressureTrend"]);

  weather->outdoor.temperature = root["Outdoor"]["Temperature"];
  weather->outdoor.minTemp = root["Outdoor"]["MinTemp"];
  weather->outdoor.maxTemp = root["Outdoor"]["MaxTemp"];
  weather->outdoor.humidity = root["Outdoor"]["Humidity"];
  strcpy(weather->outdoor.temperatureTrend, root["Outdoor"]["TemperatureTrend"]);
}

bool readData(struct Weather *weather, const char *json) {
  char *jsonCopy = strdup(json);
  
  DynamicJsonBuffer jsonBuffer;
  
  JsonObject& root = jsonBuffer.parseObject(jsonCopy);

  if (!root.success())
  {
    free(jsonCopy);

    return false;
  } else {
    extractWeather(weather, &root);
  }

  free(jsonCopy);
  
  return true;
}

void dumpWeather(struct Weather *weather) {
    Serial.println("Rain");
    
    Serial.print("Rain ");
    Serial.println(weather->rain.rain);
    Serial.print("Rain Day ");
    Serial.println(weather->rain.rainDay);
    Serial.print("Rain Hour ");
    Serial.println(weather->rain.rainHour);

    Serial.println("Wind");
    
    Serial.print("Gust ");
    Serial.println(weather->wind.gust);
    Serial.print("Max Wind ");
    Serial.println(weather->wind.maxWind);
    Serial.print("Strength ");
    Serial.println(weather->wind.strength);
    
    Serial.println("Indoor");

    Serial.print("Pressure ");
    Serial.println(weather->indoor.pressure);
    Serial.print("Humidity ");
    Serial.println(weather->indoor.humidity);
    Serial.print("Noise ");
    Serial.println(weather->indoor.noise);
    Serial.print("CO2 ");
    Serial.println(weather->indoor.co2);
    Serial.print("Temperature ");
    Serial.println(weather->indoor.temperature);
    Serial.print("Pressure Trend ");
    Serial.println(weather->indoor.pressureTrend);
    Serial.print("Min Temp ");
    Serial.println(weather->indoor.minTemp);
    Serial.print("Max Temp ");
    Serial.println(weather->indoor.maxTemp);
  
    Serial.println("Outdoor");
    
    Serial.print("Temperature ");
    Serial.println(weather->outdoor.temperature);
    Serial.print("Temperature Trend ");
    Serial.println(weather->outdoor.temperatureTrend);
    Serial.print("Humidity ");
    Serial.println(weather->outdoor.humidity);
    Serial.print("Min Temp ");
    Serial.println(weather->outdoor.minTemp);
    Serial.print("Max Temp ");
    Serial.println(weather->outdoor.maxTemp);
}

void updateData(const char *json) {
    Weather newWeather;

    if (readData(&newWeather, json)) {
        dumpWeather(&newWeather);

        memcpy(&weather, &newWeather, sizeof(Weather));
    } else {
        Serial.println("Failed to parse");
    }
}

void fetchData() {
  Serial.println("Fetching");
  
  int err = 0;

  String body = "";
  
  WiFiClient client;  
  HttpClient http(client);

  err = http.get(kHostname, kPath);

  if (err >= 0) {
    err = http.responseStatusCode();
    
    if (err >= 200 && err < 300) {
      err = http.skipResponseHeaders();

      if (err >= 0) {
        int bodyLen = http.contentLength();

        unsigned long timeoutStart = millis();
        char c;

        while (
          (http.connected() || http.available()) &&
          ((millis() - timeoutStart) < kNetworkTimeout)
          ) {

          if (http.available()) {
            c = http.read();

            body = body + c;

            bodyLen--;

            timeoutStart = millis();
          } else  {
            delay(kNetworkDelay);
          }
        }

        Serial.println("Body fetched");
        
        updateData(body.c_str());
      } else {
        Serial.print("Header skip failed");
        Serial.print(err);
      }
    } else {
      Serial.print("Fetch failed");
      Serial.print(err);
    }
  } else {
    Serial.print("Connect failed");
    Serial.print(err);
  }

  http.stop();
}

void showTemperature(Weather *weather) {
    u8g2.drawXBM(0, 12, Thermometer_width, Thermometer_height, Thermometer_bits);

    char indoorTemp[10] = { '\0' };
    char outdoorTemp[10] = { '\0' };

    dtostrf(weather->indoor.temperature,-5,1,indoorTemp);
    dtostrf(weather->outdoor.temperature,-5,1,outdoorTemp);

    u8g2.setFont(u8g2_font_helvR14_tr);

    u8g2.drawStr(16, 16, indoorTemp);
    u8g2.drawStr(16, 46, outdoorTemp);

    char indoorTempMin[10] = { '\0' };
    char indoorTempMax[10] = { '\0' };
    char outdoorTempMin[10] = { '\0' };
    char outdoorTempMax[10] = { '\0' };

    dtostrf(weather->indoor.minTemp,-5,1,indoorTempMin);
    dtostrf(weather->indoor.maxTemp,-5,1,indoorTempMax);
    dtostrf(weather->outdoor.minTemp,5,1,outdoorTempMin);
    dtostrf(weather->outdoor.maxTemp,5,1,outdoorTempMax);

    u8g2.setFont(u8g2_font_helvR08_tr);

    u8g2.drawStr(84 - u8g2.getStrWidth(indoorTempMax), 8, indoorTempMax);
    u8g2.drawStr(84 - u8g2.getStrWidth(indoorTempMin), 18, indoorTempMin);
    u8g2.drawStr(84 - u8g2.getStrWidth(outdoorTempMax), 38, outdoorTempMax);
    u8g2.drawStr(84 - u8g2.getStrWidth(outdoorTempMin), 48, outdoorTempMin);
    

    if (strcmp(weather->outdoor.temperatureTrend, "up") == 0) {
      u8g2.drawXBM(0, 0, TrendUp_width, TrendUp_height, TrendUp_bits);
    } else if (strcmp(weather->outdoor.temperatureTrend, "down") == 0) {
      u8g2.drawXBM(0, 0, TrendDown_width, TrendDown_height, TrendDown_bits);
    }
}

void showWindAndRain(Weather *weather) {
    u8g2.drawXBM(0, 6, Wind_width, Wind_height, Wind_bits);
    u8g2.drawXBM(0, 30, Rain_width, Rain_height, Rain_bits);

    char windStrength[10] = { '\0' };
    char rain[10] = { '\0' };

    dtostrf(weather->wind.strength,-5,1,windStrength);
    dtostrf(weather->rain.rain,-5,1,rain);

    u8g2.setFont(u8g2_font_helvR14_tr);

    u8g2.drawStr(16, 16, windStrength);
    u8g2.drawStr(16, 46, rain);

    char windGust[10] = { '\0' };
    char windMax[10] = { '\0' };
    char rainHour[10] = { '\0' };
    char rainDay[10] = { '\0' };

    dtostrf(weather->wind.gust,-5,1,windGust);
    dtostrf(weather->wind.maxWind,-5,1,windMax);
    dtostrf(weather->rain.rainHour,5,1,rainHour);
    dtostrf(weather->rain.rainDay,5,1,rainDay);

    u8g2.setFont(u8g2_font_helvR08_tr);

    u8g2.drawStr(84 - u8g2.getStrWidth(windMax), 8, windMax);
    u8g2.drawStr(84 - u8g2.getStrWidth(windGust), 18, windGust);
    u8g2.drawStr(84 - u8g2.getStrWidth(rainDay), 38, rainDay);
    u8g2.drawStr(84 - u8g2.getStrWidth(rainHour), 48, rainHour);

}

void showPage(Weather *weather, int page) {
  u8g2.firstPage();
  
  do {
    switch (page) {
      case 0:
        showTemperature(weather);
        break;
      case 1:
        showWindAndRain(weather);
        break;
    }
  } while ( u8g2.nextPage() );
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdateFetchMillis >= pollIntervalFetchMillis) {
    lastUpdateFetchMillis = currentMillis;

    fetchData();
  }
  
  if (currentMillis - lastUpdateScreenMillis >= pollIntervalScreenMillis) {
    lastUpdateScreenMillis = currentMillis;

    showPage(&weather, screenToShow);
    
    screenToShow = screenToShow + 1;
    
    if (screenToShow >= maxScreen) {
      screenToShow = 0;
    }
  }

  int val = digitalRead(SENSOR_PIN);
  if (val == HIGH) {
    digitalWrite(LIGHT_PIN, LOW);
  } else {
    digitalWrite(LIGHT_PIN, HIGH);
  }
}

