#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <MQ135.h>
#include <secrets.h>

#define USE_SERIAL Serial
#define DHTTYPE DHT11 // DHT 11
#define PIN_MQ135 4

char ssid[] = SECRET_SSID
char pass[] = SECRET_PASS

const int DHTPin = 1;     // DHT Sensor pin
DHT dht(DHTPin, DHTTYPE); // Initialize DHT sensor.

MQ135 mq135_sensor(PIN_MQ135);

WiFiMulti wifiMulti;
float humidity, temperature, air_quality;

void setup()
{

    USE_SERIAL.begin(115200);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for (uint8_t t = 4; t > 0; t--)
    {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    wifiMulti.addAP(SECRET_SSID, SECRET_PASS);

    dht.begin();
}

void sendHeader(const char *aHeaderName, const char *aHeaderValue);

void loop()
{
    if ((wifiMulti.run() == WL_CONNECTED)) // wait for WiFi connection
    {

        humidity = dht.readHumidity();       // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        temperature = dht.readTemperature(); // Read temperature as Celsius (the default)

        while (isnan(humidity) || isnan(temperature))
        {
            Serial.println("[ERROR] Failed to read from DHT sensor!");

            humidity = dht.readHumidity();
            delay(1000);
            temperature = dht.readTemperature();
        }

        air_quality = analogRead(PIN_MQ135);
        Serial.println(air_quality, DEC);

        float rzero = mq135_sensor.getRZero();
        float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, humidity);
        float resistance = mq135_sensor.getResistance();
        float ppm = mq135_sensor.getPPM();
        float correctedPPM = mq135_sensor.getCorrectedPPM(temperature, humidity);

        Serial.print("MQ135 RZero: ");
        Serial.print(rzero);
        Serial.print("\t Corrected RZero: ");
        Serial.print(correctedRZero);
        Serial.print("\t Resistance: ");
        Serial.print(resistance);
        Serial.print("\t PPM: ");
        Serial.print(ppm);
        Serial.print("\t Corrected PPM: ");
        Serial.print(correctedPPM);
        Serial.println("ppm");

        HTTPClient http;

        // configure traged server and url
        http.begin("https://meteoauth.tk/api/measured_values/create");
        String bearerToken = "Bearer eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxIiwiaWF0IjoxNjQ5MTcwNDY5LCJhdXRob3JpdGllcyI6IlNUQVRJT05fUk9MRSJ9.j0wip3orQOKozLVctFnEIOEpCJ7uu9VVB-SC9a5YaZM";
        http.addHeader("Authorization", bearerToken);
        http.addHeader("Content-Type", "application/json");

        // start connection and send HTTP header

        StaticJsonDocument<192> doc;

        doc["humidity"] = humidity;
        doc["temperature"] = temperature;
        doc["air_quality"] = air_quality;
        doc["wind_speed"] = "";
        doc["wind_gusts"] = "";
        doc["wind_direction"] = "";
        doc["rainfall"] = "";

        String requestBody;
        serializeJson(doc, requestBody);

        int httpCode = http.POST(requestBody);

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {
                String payload = http.getString();
                USE_SERIAL.printf("[RESPONSE] %s\n", payload.c_str());
            }
        }
        else
        {
            USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
    }

    delay(600000);
    //delay(9000);
}