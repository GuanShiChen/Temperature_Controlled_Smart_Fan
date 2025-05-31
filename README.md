# Smart Fan System with Temperature-Based Control

## Overview:
The goal of this project is to create a smart fan system that adjust its speed based on the current temperature. There are two modes: Manual and Automatic.
- **Manual**: The fan adjusts its speed based on the position of a potentiometer knob.
- **Automatic**: The fan adjusts its speed based on the temperature retrieved from either a local temperature sensor or the OpenWeatherMap API.

Compared to traditional fans, the smart fan aims to implement automation into simple fan systems, making it easier to use and convenient for the user.
The project demonstrates embedded systems concepts such as sensor interfacing, analog and digital I/O, serial communication, API usage, and PWM control.

---

## Components:
1. **12V Power Supply**:
This provides the main power for the entire system, including the fan and the controller.

2. **Buck Converter**:
The buck converter steps down the 12V from the power supply to 5V to be used by the ESP8266 module, Arduino Mega, and sensors.

3. **Arduino Mega 2560 R3**:
The Arduino Mega connects all peripherals of the system. The PWM pin is used to control the fan. The digital pins interfaces with push buttons, switch, serial displays, humidity sensor, and the transistor. The analog pins interface with the temperature sensor and the potentiometer. The TX/RX serial communication pins interfaces with the ESP8266.

4. **ESP8266 Wi-Fi Module**:
The ESP8266 connects to the internet via WiFi and retrieves temperature and humidity data from the OpenWeatherMap API in automatic mode. The TX/RX serial communication is used to send the weather data to the Arduino Mega.

5. **DHT11 Sensor**:
When the data source is set to "Local Sensor", the Arduino Mega retreives humidity information from the DHT11 sensor for display. The sensor is a common way to measure temperature and humidity, but using a dedicated temperature (like TMP36) will result in temperature readings of higher accuracy.

6. **TMP36 Sensor**:
An analog temperature sensor to retreive ambient temperature reading. The sensor is used as an alternative to the DHT11 for more accurate temperature readings.

7. **Potentiometer**:
The potentiometer allows manual control of fan speed in manual mode.

8. **Push Buttons**:
The push buttons are used for making selections in the menu and increasing/decreasing selected values.

9. **Switch**:
The switch is used to switch between the automatic and manual mode.

10. **1602A Serial Displays**:
The displays are used to display current system status, such as temperature, humidity, mode, and fan speed. The dispay also shows a menu for the user to adjust the settings of the system.

11. **2N2222 (NPN Transistor)**:
The transistor acts as a switch to control power to the 12V DC fan using PWM from the Arduino. Since the fan still spins at a low speed when PWM input is 0, the transistor is used to cut power when the fan speed is set to 0.

12. **1N4001 (Diode)**:
The diode acts as a flyback diode to prevent voltage spikes when an inductive load, motor coil inside the fan, is suddenly de-energized.

13. **12V DC Fan**:
The fan uses electrical energy and converts it into mechanical movement of air molecules. The fan runs on 12V for power and a PWM signal pin for adjusting fan speed.

14. **Resistors**:
One resistor is used to pull the signal pin of the DHT11 high. Another resistor is used to control the current going into the base of the NPN transistor.

---
## Wiring Diagram
![Schematic](/Wiring%20Diagram.png)

---

## System Operation
On startup, the ESP8266 connects to the Wi-Fi network (SSID and password are flashed in firmware) and fetches weather data from the OpenWeatherMap API. The weather data obtained is sent to Arduino Mega through Serial1 (using hardware TX and RX pins).

In the menu, the user is able to choose their data source, whether it be from the weather API or local sensors. The data from the corresponding sources are displayed and used.

In the **automatic mode**, the fan speed is based on where the temperature is within a range. The user is able to adjust the maximum and minimum temperature of the range in the menu using the push buttons. The exact speed for the fan is calculated and the Arduino generates a PWM signal that corresponds to the correct speed. In the **manual mode**, the system bypasses temperature input and instead uses a potentiometer to set the fan speed.

---

## Pictures
<img src="/Pictures/20250519_221357.jpg" alt="Overview" width="500"><br>
<img src="/Pictures/20250519_221528.jpg" alt="Manual Source Select" width="500"><br>
<img src="/Pictures/20250519_222247.jpg" alt="Auto Menu" width="500"><br>


For **Videos**, visit [Smart Fan Videos - Google Drive](https://drive.google.com/drive/folders/1x3JrscKStHZuTtQESSH0A9cGB1mKeb2B?usp=drive_link)

---

## Challenges Faced:
Several challenges were faced while completing the project:
#### 1. Making Weather API Calls:
I originally planned on using the data from the National Weather Service (NWS) provided by the government as there was no limit on the number of API calls. Making the HTTP request worked, but I did not receive the correct response from the data due to problems with web headers. I then switched to OpenWeather API as making the API call only took one http request as opposed to two for NWS, but there was a limit on the number of requests per day. I had to add a delay between API calls, which comes at a cost of not having the most accurate information until the next call is made.
#### 2. Integrating Transistor:
With the fan I had, I did not realize that sending nothing on the PWM pin to the fan still results in the fan spinning at a slow speed. With my idea to turn off the fan when the speed is 0, I had to learn how bi-polar junction transistors worked to stop the power to the fan with a digital pin from the Arduino.
#### 3. Sensor Accuracy:
The sensors were very limited in terms of their accuracy. The temperature readings from the DHT11 sensor would fluctuate in the degrees range within the span of one second. I decided to opt for the TMP36 sensor as it had higher accuracy and lower variability, but the data produced still varies within a small time frame.
#### 4. Serial Communication:
It was difficult to debug when something went wrong with the serial communication between the ESP8266 and the Arduino since there was not a way to view whether the data has been sent or whether the data was received. I also had problems when Serial and Serial1 was set at different baud rates where data would not be sent properly.
#### 5. Display Flickering:
If the display is being costantly cleared and written each time the loop runs, the display will visibly flicker, giving the users a bad experience. I added additional code to check whether if anything on the screen changes or changing the interval for when the displayed elements are updated.

---

## Real-World Comparisons
### Smart Thermostat
Smart thermostats in homes adjusts its heating and cooling of a house based on the current temperature, just like the fan base its fan speed on the temperature. My fan differs from smart thermostats in the sense that the actuation system is different (heating/cooling element vs fan). My fan also has a manual mode allowing the user to control the system, whereas most consumer thermostats do not have that feature.
### Smart Fans
There are many consumer fans and smart fans available with the idea of basing fan speed off of ambient temperature. My fan differs in the way that the manual mode can be adjusted with a potentiometer, making it a more hands-on experience and more control for the user.

---

## Future Improvements
If I had more time, I would like to add a menu setting allowing users to change their preferred units (C vs F for temperature). I would also like to create a mobile app or web interface to allow the user to control the fan wirelessly. I could also integrate with an sd card to store the ambient temperature data, allowing users to see the temperature and humidity trend of their surrounding. Lastly, in case of power outage, a battery backup system would allow the device to work without being plugged in, allowing for the device to be portable.

---

## Code Library Reference
The libraries used makes it easier and simpler to develop the code for the project. 

#### LiquidCrystal (Display API)
https://github.com/arduino-libraries/LiquidCrystal

Provides an easy way to interface with the displays by simply setting a cursor for the location and printing plain text or numbers directly to the display.

#### DHT (Humidity Sensor API)
https://github.com/adafruit/DHT-sensor-library

Simplifies the commuinication with the humidity sensor. The provided function to retrieve humidity data in the library can be called without having to do memory mapping and bit masking, making the process simpler.

#### ESP8266WiFi
https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi

Provides a straightforward way of managing WiFi connections on the ESP8266-based microcontrollers, allowing the developer to easily make wifi connections, retrieve connection status, and manage network settings.

#### ESP8266HTTPClient
https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient

Provides a way to perform HTTP operations (GET, POST, PUT, DELETE, etc.) over WiFi using the ESP8266 module. It supports the handling of sending requests and receiving responses over the web using the HTTP protocol.

#### Arduino JSON
https://github.com/arduino-libraries/Arduino_JSON

An efficient JSON library that simplifies the process of parsing and generating JSON on Arduino microcontrollers, making it easier for reading information from API calls and handling sensor data.

*Library files are provided in the `/Libraries/` directory
