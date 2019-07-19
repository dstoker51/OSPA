/* 
 * ESP32 NodeMCU DHT11 - Humidity Temperature Sensor Example
 * https://circuits4you.com
 * 
 * References
 * https://circuits4you.com/2017/12/31/nodemcu-pinout/
 * 
 */
 
#include "DHTesp.h"
#include "WiFi.h"

#define DHTpin 27    //D15 of Sparkfun ESP32 Thing
#define SOIL_SENSOR 36
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

const char* ssid     = "Bridge Four";
const char* password = "TheLopen4";

RTC_DATA_ATTR int bootCount = 0;
int soil_amount;
float humidity;
float temperature;

WiFiServer server(80);
DHTesp dht;

void setup()
{
  pinMode(SOIL_SENSOR, INPUT);
  
  delay(10);

  // Set up the DHT.
//  dht.setup(DHTpin, DHTesp::DHT11); //for DHT11
  dht.setup(DHTpin, DHTesp::DHT22); //for DHT22

  // Set up serial connection.
  Serial.begin(115200);
  Serial.println();

  // Wifi Server setup.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();

  // Deep sleep setup.
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void loop()
{
  // Check for wifi client.
  WiFiClient client = server.available();   // Listen for incoming clients.

  // Read sensor values.
  soil_amount = analogRead(SOIL_SENSOR);  // Query soil moisture sensor.
  humidity = dht.getHumidity();           // Query humidity from DHT.
  temperature = dht.getTemperature();     // Query temperature from DHT.

  // Print data if the client is connected.
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected  
        // Print moisture data.
        Serial.println("------------------------------------------------------------------------------------------");
        Serial.print("Soil moisture level: ");
        Serial.println(soil_amount);
      
        // Print humity and temperature data.
        Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
        Serial.print(dht.getStatusString());
        Serial.print("\t");
        Serial.print(humidity, 1);
        Serial.print("\t\t");
        Serial.print(temperature, 1);
        Serial.print("\t\t");
        Serial.print(dht.toFahrenheit(temperature), 1);
        Serial.print("\t\t");
        Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
        Serial.print("\t\t");
        Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
        Serial.println("------------------------------------------------------------------------------------------");
        Serial.println();
      }
  }

  // Delay until the next run.
  Serial.println("Going to sleep.");
  Serial.flush(); 
  esp_deep_sleep_start();
}
