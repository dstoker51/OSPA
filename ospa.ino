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
#define SOIL_SENSOR 38
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300       /* Time ESP32 will go to sleep (in seconds) */

// Network
const char* ssid     = "shadowsofsilence";
const char* password = "intheforestsofhell";

// InfluxDB
const char* sensor_id = "0x0003";
const char* influxdb_query_url="http://172.16.0.1:8086/write?db=ospa_test1&u=bot&p=allyourbasearebelongtous";

RTC_DATA_ATTR int bootCount = 0;
float moisture_level;
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

// Requires esp_sleep_enable_timer_wakeup to have been set.
void go_to_deep_sleep() {
  Serial.println("Going to sleep.");
  Serial.flush(); 
  esp_deep_sleep_start();
}

// Adds a 1ms delay.
float get_averaged_soil_moisture(int num_samples) {
  if(num_samples <= 0) {
    return 0;
  }

  float average = analogRead(SOIL_SENSOR);
  for(int i=0; i<num_samples; i++) {
    float moisture_level = analogRead(SOIL_SENSOR); // Query soil moisture sensor.
    average = (average + moisture_level) * 0.5;
    delay(1);
  }
  return average;
}

void setup()
{
  // Deep sleep setup.
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Set soil sensor pins.
  pinMode(SOIL_SENSOR, INPUT);
  delay(10);

  // Set up the DHT.
  dht.setup(DHTpin, DHTesp::DHT22); //for DHT22

  // Set up serial connection.
  Serial.begin(115200);
  Serial.println();

  // Wifi Server setup.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Even though this is a long-running system, there is no need to worry about overflow due to the 
  // deep sleep mode we are using. Millis will reset each time the system goes to sleep. 
  unsigned long start_time = millis();
  unsigned long current_time = millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");

      // If the WiFi attempt timeout has been reached then go back to sleep.
      current_time = millis();
      if (current_time - start_time > 30000) {
        Serial.println("");
        Serial.println("WiFi connection failed. Continuing, but no WiFi data will be sent.");
        break;
      }
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop()
{
  // Read sensor values.
  moisture_level = get_averaged_soil_moisture(100); // Query soil moisture sensor.
  humidity = dht.getHumidity();   // Query humidity from DHT.
  temp_c = dht.getTemperature();  // Query temperature from DHT.
  float temp_f = dht.toFahrenheit(temp_c);  // Convert to Fahrenheit.

  // Check for wifi client.
  WiFiClient client = server.available();   // Listen for incoming clients.

  serial_print_sensor_values();

  // Print data if the client is connected.
  if (client) {                             // If you get a client,
    Serial.println("New Client.");          // print a message out the serial port.
    String currentLine = "";                // Make a String to hold incoming data from the client.
    while (client.connected()) {            // Loop while the client's connected.
        client_print_sensor_values(client);
        delay(500);
    } 
  }

  // Send data to InfluxDB.
  if(WiFi.status()== WL_CONNECTED) {   // Check WiFi connection status.
    // Create HTTP request.
    HTTPClient http;
    http.begin(influxdb_query_url);    // Specify request destination.
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  // Specify content-type header.
  
    // Generate the request payload and send it out.
    int httpCode = http.POST(String("sensor_data,field_id=KaysvilleCoverCropPlots") + " humidity=" + humidity + ",temp_c=" + temp_c + ",temp_f=" + temp_f + ",soil_moisture_level=" + moisture_level);   
    String response_payload = http.getString(); // Get the response payload.
    Serial.println(httpCode);   // Print HTTP return code.
    Serial.println(response_payload); //Print request response payload.

    // Close connection
    http.end(); 
  }
  else {
    Serial.println("Error in WiFi connection.");   
  }

  // Delay until the next run.
  go_to_deep_sleep();
}
