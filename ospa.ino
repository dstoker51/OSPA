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

const char* ssid     = "Bridge Four";
const char* password = "TheLopen4";

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
}

void loop()
{
  // Check for wifi client.
  WiFiClient client = server.available();   // Listen for incoming clients.
  
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
//      if (client.available()) {             // if there's bytes to read from the client,
//        char c = client.read();             // read a byte, then
//        Serial.write(c);                    // print it out the serial monitor
//        if (c == '\n') {                    // if the byte is a newline character
//  
//          // if the current line is blank, you got two newline characters in a row.
//          // that's the end of the client HTTP request, so send a response:
//          if (currentLine.length() == 0) {
//            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//            // and a content-type so the client knows what's coming, then a blank line:
//            client.println("HTTP/1.1 200 OK");
//            client.println("Content-type:text/html");
//            client.println();
//  
//            // the content of the HTTP response follows the header:
//            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
//            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");
//  
//            // The HTTP response ends with another blank line:
//            client.println();
//            // break out of the while loop:
//            break;
//          } else {    // if you got a newline, then clear currentLine:
//            currentLine = "";
//          }
//        } else if (c != '\r') {  // if you got anything else but a carriage return character,
//          currentLine += c;      // add it to the end of the currentLine
//        }
  
        // Query soil moisture sensor.
        int soil_amount = analogRead(SOIL_SENSOR);
      
        // Print the data.
        client.println("------------------------------------------------------------------------------------------");
        client.print("Soil moisture level: ");
        client.println(soil_amount);
      
        // Query humidity/temperature sensor.
        float humidity = dht.getHumidity();
        float temperature = dht.getTemperature();
      
        // Print the data.
        client.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");
        client.print(dht.getStatusString());
        client.print("\t");
        client.print(humidity, 1);
        client.print("\t\t");
        client.print(temperature, 1);
        client.print("\t\t");
        client.print(dht.toFahrenheit(temperature), 1);
        client.print("\t\t");
        client.print(dht.computeHeatIndex(temperature, humidity, false), 1);
        client.print("\t\t");
        client.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
        client.println("------------------------------------------------------------------------------------------");
        client.println();
      
        // Delay until the next run.
        delay(2000);
      }
//    }
  }
}
