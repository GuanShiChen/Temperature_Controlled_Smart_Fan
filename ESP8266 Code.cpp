#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>

// WiFi credentials
const char* ssid = "myHotspot";
const char* password = "11111111";

// OpenWeatherMap API key and city details
String openWeatherMapApiKey = "80b6ee5a5119350f4e47ac42a596f27b";
String city = "Atlanta";
String countryCode = "US";

// Timer settings for making API calls every 20 seconds
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;  // 20 seconds

String jsonBuffer;

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  Serial1.begin(115200);   // Communication with Arduino Uno

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi, IP Address: ");
  Serial.println(WiFi.localIP());

  // Set timer for 20 seconds
  int sec = timerDelay / 1000;
  Serial.println("Timer set to " + String(sec) + " seconds.");
}

void loop() {
  // Check if it's time to request new weather data
  if ((millis() - lastTime) > timerDelay) {
    // Only fetch data if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
      // Construct the OpenWeatherMap API URL with the city and API key
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      
      // Send GET request to OpenWeatherMap and receive JSON response
      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // If JSON parsing fails, print an error and return
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      // Extract temperature (in Kelvin), humidity, pressure, and wind speed from JSON response
      double tempK = myObject["main"]["temp"];
      double tempC = tempK - 273.15;                       // Convert to Celsius
      double tempF = (tempK - 273.15) * 9.0 / 5.0 + 32.0;  // Convert to Fahrenheit

      // Format weather data as a comma-separated string
      String weatherData = String(tempC, 2) + "," +
        String(tempF, 2) + "," +
        String((int)myObject["main"]["humidity"]) + "," +
        String((int)myObject["main"]["pressure"]) + "," +
        String((double)myObject["wind"]["speed"], 2);

      // Send weather data to Arduino Uno via Serial1
      Serial1.println(weatherData);

      // Print weather data to Serial Monitor for debugging
      Serial.println(weatherData);
    }
    else {
      // Print message if WiFi is disconnected
      Serial.println("WiFi Disconnected");
    }
    // Update the last time to the current time
    lastTime = millis();
  }
}

// Function to send GET request to the server and return the response payload
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Initiate the HTTP request
  http.begin(client, serverName);
  
  // Send GET request
  int httpResponseCode = http.GET();
  
  // Default empty JSON payload in case of error
  String payload = "{}"; 
  
  // If the response code is positive, retrieve the response content
  if (httpResponseCode > 0) {
    payload = http.getString();
  }
  else {
    // Print error code if the request fails
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // End the HTTP request
  http.end();

  // Return the response payload
  return payload;
}
