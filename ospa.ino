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

// Deep sleep
RTC_DATA_ATTR int bootCount = 0;

// Network
const char* wifi_ssid     = "shadowsofsilence";
const char* wifi_password = "intheforestsofhell";

// InfluxDB
String database_url = "172.16.0.1";
String database_port = "8086";
String database_user = "bot";
String database_password = "allyourbasearebelongtous";
String soil_moisture_database = "darinshouse_soil_moisture";
String temp_humidity_database = "darinshouse_temp_humidity";
String field_id = "DarinsBackyard";
String board_id = "esp32-0002";
String soil_moisture_sensor_id = "csms-0001";
String temp_humidity_sensor_id = "dht22-0001";
String soil_moisture_query_url="http://" + database_url + ":" + database_port + "/write?db=" + soil_moisture_database + "&u=" + database_user + "&p=" + database_password;
String temp_humidity_query_url="http://" + database_url + ":" + database_port + "/write?db=" + temp_humidity_database + "&u=" + database_user + "&p=" + database_password;

// Sensors
DHTesp dht;

void serial_print_sensor_values(float soil_moisture, float humidity, float temp_c, float temp_f) {
  // Print moisture data.
  Serial.println("------------------------------------------------------------------------------------------");
  Serial.print("Soil moisture level: ");
  Serial.println(soil_moisture);

  // Print humity and temperature data.
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temp_c, 1);
  Serial.print("\t\t");
  Serial.print(temp_f, 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temp_c, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(temp_f, humidity, true), 1);
  Serial.println("------------------------------------------------------------------------------------------");
  Serial.println();
}

String construct_line_protocol_for_soil_moisture_sensor(char* sensor_id, float soil_moisture_level) {
  // TODO: Get rid of String objects.
  return String("sensor_data,field_id=" + field_id) + "sensor_id=" + sensor_id + ",soil_moisture_level=" + moisture_level;
}

String construct_line_protocol_for_temp_humidity_sensor(char* sensor_id, float humidity, float temp_c, float temp_f) {
  // TODO: Get rid of String objects.
  return String("sensor_data,field_id=" + field_id) + " sensor_id=" + sensor_id + ",humidity=" + humidity + ",temp_c=" + temp_c + ",temp_f=" + temp_f;
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
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

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
  float moisture_level = get_averaged_soil_moisture(100); // Query soil moisture sensor.
  float humidity = dht.getHumidity();   // Query humidity from DHT.
  float temp_c = dht.getTemperature();  // Query temperature from DHT.
  float temp_f = dht.toFahrenheit(temp_c);  // Convert to Fahrenheit.

  serial_print_sensor_values();

  // Send data to InfluxDB.
  if(WiFi.status()== WL_CONNECTED) {   // Check WiFi connection status.
    // Create HTTP request.
    HTTPClient http;
    http.begin(influxdb_query_url);    // Specify request destination.
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");  // Specify content-type header.
  
    // Send the soil moisture sensor data.
    int httpCode = http.POST(construct_line_protocol_for_soil_moisture_sensor(soil_moisture_sensor_id, moisture_level));   
    String response_payload = http.getString(); // Get the response payload.
    Serial.println(httpCode);   // Print HTTP return code.
    Serial.println(response_payload); //Print request response payload.

    // Send the temp/humidity sensor data.
    int httpCode = http.POST(construct_line_protocol_for_temp_humidity_sensor(temp_humidity_sensor_id, humidity, temp_c, temp_f));   
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
