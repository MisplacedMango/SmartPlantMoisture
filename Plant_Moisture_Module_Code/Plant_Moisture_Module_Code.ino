
//ESP32 libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "DHT.h"

//Wifi stuff
const char* ssid = "KruuseNet";
const char* password = "NetKruuse";
int port = 80;
WebServer server(port);

const int led = LED_BUILTIN;

//DHT11 things
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Other things
float asoilmoist = analogRead(32);  //global variable to store exponential smoothed soil moisture reading

void handleRoot() {
  digitalWrite(led, 1);

  String webtext;
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  float hum = dht.readHumidity();
  // Read temperature as Celsius
  float temp = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Soil moisture evaluation
  String moistureStatus, moistureColor;
  if (asoilmoist > 3200) {
    moistureStatus = "Way too dry";
    moistureColor = "red";
  } else if (asoilmoist > 2800) {
    moistureStatus = "Too dry";
    moistureColor = "orange";
  } else if (asoilmoist > 2400) {
    moistureStatus = "Good (Moist)";
    moistureColor = "green";
  } else if (asoilmoist > 2000) {
    moistureStatus = "Wet";
    moistureColor = "orange";
  } else {
    moistureStatus = "Too wet";
    moistureColor = "red";
  }
  // Temperature evaluation
  String tempStatus, tempColor;
  if (temp >= 20 && temp <= 30) {
    tempStatus = "Perfect";
    tempColor = "green";
  } else if ((temp >= 15 && temp < 20) || (temp > 30 && temp <= 35)) {
    tempStatus = "Tolerable";
    tempColor = "orange";
  } else if ((temp >= 10 && temp < 15) || (temp > 35 && temp <= 38)) {
    tempStatus = "Stressing";
    tempColor = "red";
  } else {
    tempStatus = "Killing";
    tempColor = "red";
  }

  // Humidity evaluation
  String humStatus, humColor;
  if (hum >= 30 && hum <= 50) {
    humStatus = "Perfect";
    humColor = "green";
  } else if (hum > 50 && hum <= 60) {
    humStatus = "Tolerable";
    humColor = "orange";
  } else if (hum > 60 && hum <= 70) {
    humStatus = "Stressing";
    humColor = "red";
  } else {
    humStatus = "Killing";
    humColor = "darkred";
  }

  webtext = "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Aloe Vera Plant Monitor</title>\
    <style>\
      body { background-color: #f0f0f0; font-family: Arial; color: #333; }\
      .green { color: green; }\
      .orange { color: orange; }\
      .red { color: red; }\
      .darkred { color: darkred; }\
    </style>\
  </head>\
  <body>\
    <h1>Aloe Vera Monitoring</h1>\
    <p><strong>Soil Moisture:</strong> <span class='"
            + moistureColor + "'>" + String(asoilmoist) + " (" + moistureStatus + ")</span></p>\
    <p><strong>Temperature:</strong> <span class='"
            + tempColor + "'>" + String(temp) + "&#176;C (" + tempStatus + ")</span></p>\
    <p><strong>Humidity:</strong> <span class='"
            + humColor + "'>" + String(hum) + "% (" + humStatus + ")</span></p>\
    <p>Date/Time: <span id='datetime'></span></p>\
    <script>\
      var dt = new Date();\
      document.getElementById('datetime').innerHTML = (('0'+dt.getDate()).slice(-2)) + '.' + (('0'+(dt.getMonth()+1)).slice(-2)) + '.' + dt.getFullYear() + ' ' + (('0'+dt.getHours()).slice(-2)) + ':' + (('0'+dt.getMinutes()).slice(-2));\
    </script>\
  </body>\
</html>";

  server.send(200, "text/html", webtext);
  digitalWrite(led, 0);
}


void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
  delay(1000);
  digitalWrite(led, 1);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  dht.begin();
  delay(2000);
}

void loop(void) {
  asoilmoist = 0.95 * asoilmoist + 0.05 * analogRead(32);  //exponential smoothing of soil moisture
  server.handleClient();

  // Read sensors
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  delay(2000);
}