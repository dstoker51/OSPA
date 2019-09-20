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
#include <HTTPClient.h>

#define DHTpin 27    //D15 of Sparkfun ESP32 Thing
#define SOIL_SENSOR 36
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

// Network
const char* ssid     = "Bridge Four";
const char* password = "TheLopen4";

// InfluxDB
const char* sensor_id = "0x0001";
const char* influxdb_query_url="http://192.168.1.131:8086/write?db=ospa_test1&u=bot&p=allyourbasearebelongtous";

RTC_DATA_ATTR int bootCount = 0;
int moisture_level;
float humidity;
float temp_c;

WiFiServer server(80);
DHTesp dht;

void serial_print_sensor_values() {
  // Print moisture data.
  Serial.println("------------------------------------------------------------------------------------------");
  Serial.print("Soil moisture level: ");
  Serial.println(moisture_level);

  // Print humity and temperature data.
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temp_c, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temp_c), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temp_c, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temp_c), humidity, true), 1);
  Serial.println("------------------------------------------------------------------------------------------");
  Serial.println();
}

void client_print_sensor_values(WiFiClient &client) {
  // Print moisture data.
  client.println("------------------------------------------------------------------------------------------");
  client.print("Soil moisture level: ");
  client.println(moisture_level);

  // Print humity and temperature data.
  client.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  client.print(dht.getStatusString());
  client.print("\t");
  client.print(humidity, 1);
  client.print("\t\t");
  client.print(temp_c, 1);
  client.print("\t\t");
  client.print(dht.toFahrenheit(temp_c), 1);
  client.print("\t\t");
  client.print(dht.computeHeatIndex(temp_c, humidity, false), 1);
  client.print("\t\t");
  client.println(dht.computeHeatIndex(dht.toFahrenheit(temp_c), humidity, true), 1);
  client.println("------------------------------------------------------------------------------------------");
  client.println();
}

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
//  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

void loop()
{
  // Read sensor values.
  moisture_level = analogRead(SOIL_SENSOR);  // Query soil moisture sensor.
  humidity = dht.getHumidity();           // Query humidity from DHT.
  temp_c = dht.getTemperature();     // Query temperature from DHT.
  float temp_f = dht.toFahrenheit(temp_c);     // Convert to fahrenheit.

  // Check for wifi client.
  WiFiClient client = server.available();   // Listen for incoming clients.

  serial_print_sensor_values();

  // Print data if the client is connected.
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected  
        client_print_sensor_values(client);
        delay(500);
    } 
  }

  // Send data to InfluxDB.
  if(WiFi.status()== WL_CONNECTED) {   // Check WiFi connection status
    // Create HTTP request
    HTTPClient http;
    http.begin(influxdb_query_url);      // Specify request destination
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  // Specify content-type header
  
    // Generate the request payload and send it out.
    int httpCode = http.POST(String("sensor_data,field_id=KaysvilleCoverCropPlots") + " humidity=" + humidity + ",temp_c=" + temp_c + ",temp_f=" + temp_f + ",soil_moisture_level=" + moisture_level);   
    String response_payload = http.getString(); //Get the response payload
    Serial.println(httpCode);   // Print HTTP return code
    Serial.println(response_payload); //Print request response payload

    // Close connection
    http.end(); 
  }
  else {
    Serial.println("Error in WiFi connection.");   
  }

  // Delay until the next run.
  delay(500); // TODO Use deep sleep instead. 
//  Serial.println("Going to sleep.");
//  Serial.flush(); 
//  esp_deep_sleep_start();
}
