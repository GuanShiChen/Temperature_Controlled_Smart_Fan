// Include necessary libraries
#include "DHT.h"
#include <LiquidCrystal.h>

// Define delay constants
#define debounceDelay 100
#define tempDelay 2000
#define fanSpeedUpdateDelay 1000

// DHT11 Config
#define DHTPIN 41
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);     // Initialize DHT11 sensor

// Pin assignments
const int tempPin = A14;       // Analog pin for TMP36 temperature sensor
const int potPin = A15;        // Analog pin for potentiometer (manual mode)
const int switchPin = 12;      // Mode switch pin (automatic/manual)
const int fanPin = 13;         // PWM pin to control fan
const int transistorPin = 2;   // Digital pin to activate/deactivate fan transistor
const int enterPin = 3;        // Digital pin for enter button
const int upPin = 4;           // Digital pin for up button
const int downPin = 5;         // Digital pin for down button

// Temperature range settings (in Celsius)
float minTemp = 20.0;    // Minimum temp for fan control
float maxTemp = 35.0;    // Maximum temp for fan control

// Fan control variables
int fanSpeed;
int fanSpeedVal;
float temperature = 999;
float prevTemp = temperature;

// Mode (0 = auto, 1 = manual)
int mode = 0;

// LCD display setup
LiquidCrystal lcd(6, 7, 8, 9, 10, 11);         // Display 1: Fan info (fan speed & mode)
LiquidCrystal lcd2(53, 52, 51, 50, 49, 48);    // Display 2: Temperature and humidity
LiquidCrystal lcd3(47, 46, 45, 44, 43, 42);    // Display 3: Menu

// Menu navigation state
bool enterButtonState = HIGH;
bool prevEnterButtonState = HIGH;
bool upButtonState = HIGH;
bool prevUpButtonState = HIGH;
bool downButtonState = HIGH;
bool prevDownButtonState = HIGH;
int state = 0;
int prevState = 0;

// Configuration variables (live and previous)
float setMaxTemp = maxTemp;
float setMinTemp = minTemp;
float prevSetMaxTemp = setMaxTemp;
float prevSetMinTemp = setMinTemp;

int prevSwitchState = 999;
int prevFanSpeedVal = 999;
int prevMode = 999;

// Timing
unsigned long prevTempTime = 0;
float tempDisplay = 0;
int prevDisplayState = 999;
unsigned long fanSpeedUpdate = 0;

// Pointer to returned temperature array
float* readVal;

// Data source toggle (false = local, true = API)
bool dataSource = false;


// -------------------- Setup --------------------

void setup() {
  // Initialize LCD displays
  lcd.begin(16, 2);
  lcd2.begin(16, 2);
  lcd3.begin(16, 2);

  // Set pin modes
  pinMode(fanPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(enterPin, INPUT_PULLUP);
  pinMode(upPin, INPUT_PULLUP);
  pinMode(downPin, INPUT_PULLUP);
  pinMode(transistorPin, OUTPUT);
  digitalWrite(transistorPin, HIGH); // Enable fan transistor initially

  // Start serial for debugging and communication with ESP8266
  Serial.begin(115200);
  Serial1.begin(115200);

  // Start DHT sensor
  dht.begin();
}

// -------------------- Temperature Reading Function --------------------

// Returns pointer to array [tempC, tempF, humidity]
float* readTemperature() {
  float tempC = 999;
  float tempF = 999;
  float humidity = 999;
  static float returnVal[3];

  if (dataSource) {
    // Read from ESP8266 over Serial1
    if (Serial1.available()) {
      String data = Serial1.readStringUntil('\n');
      Serial.println("Received from ESP8266: " + data);

      // Check WiFi connection (if data contains space, wifi is disconnected, and visa versa)
      if (data.indexOf(" ") == -1) {
        float values[5];
        int startIndex = 0;
        int endIndex;
        int valueIndex = 0;

        // Parse comma-separated data
        while ((endIndex = data.indexOf(',', startIndex)) != -1) {
          String value = data.substring(startIndex, endIndex);
          values[valueIndex++] = value.toFloat();
          startIndex = endIndex + 1;
        }

        values[valueIndex] = data.substring(startIndex).toFloat(); // Last value

        // Assign values
        tempC = values[0];
        tempF = values[1];
        humidity = values[2];
        int pressure = values[3];
        int windSpeed= values[4];

        Serial.println("---------------------------------");
        Serial.println("Temp C: " + String(tempC));
        Serial.println("Temp F: " + String(tempF));
        Serial.println("Humidity: " + String(humidity));
        Serial.println("Pressure: " + String(pressure));
        Serial.println("Wind Speed: " + String(windSpeed));
        
        returnVal[0] = tempC;
        returnVal[1] = tempF;
        returnVal[2] = humidity;
        Serial.println("returnVal[0]: " + String(returnVal[0]));
        Serial.println("returnVal[1]: " + String(returnVal[1]));
        Serial.println("returnVal[2]: " + String(returnVal[2]));
      }
      else {
        // Wifi is disconnected or invalid data
        returnVal[0] = 999;
        returnVal[1] = 999;
        returnVal[2] = 999;
        Serial.println("WiFi disconnected or bad format");
      }
    }
  }
  else {
    // Local TMP36 temperature sensor and DHT11 humidity
    int sum = 0;
    for (int i = 0; i < 5; i++) {
      sum += analogRead(tempPin);
    }

    sum /= 5;
    float voltage = sum * (5.0 / 1023.0);
    tempC = (voltage - 0.5) * 100;           // Convert to Celsius
    tempF = tempC * 9.0 / 5.0 + 32.0;        // Convert to Fahrenheit
    humidity = dht.readHumidity();           // Read humidity

    returnVal[0] = tempC;
    returnVal[1] = tempF;
    returnVal[2] = humidity;
  }

  return returnVal;
}

void loop() {
  unsigned long startTime = millis();

  // -------------------- Mode Switch Handling --------------------

  int switchState = digitalRead(switchPin); // Read current switch state (manual/auto)

  if (switchState != prevSwitchState) {
    if (switchState == LOW) {
      // Switched to AUTOMATIC
      prevSwitchState = switchState;
      mode = 0;
      if (state == 0 || state == 9 || state == 10) {
        prevState = 1;
        state = 1;
      }
      state = prevState;
      delay(debounceDelay);
    } else {
      // Switched to MANUAL
      prevSwitchState = switchState;
      mode = 1;
      if (state >= 1 && state <= 8) {
        prevState = 0;
        state = 0;
      }
      state = prevState;
      delay(debounceDelay);
    }
  }

  // -------------------- Read Button States --------------------

  enterButtonState = digitalRead(enterPin);
  upButtonState = digitalRead(upPin);
  downButtonState = digitalRead(downPin);

  if (enterButtonState == HIGH && prevEnterButtonState == LOW) {
    prevEnterButtonState = HIGH;
    delay(debounceDelay);
  }
  if (upButtonState == HIGH && prevUpButtonState == LOW) {
    prevUpButtonState = HIGH;
    delay(debounceDelay);
  }
  if (downButtonState == HIGH && prevDownButtonState == LOW) {
    prevDownButtonState = HIGH;
    delay(debounceDelay);
  }

  // -------------------- AUTOMATIC MODE --------------------
  if (mode == 0) {
    readVal = readTemperature();
    float readTempC = readVal[0];
    // If valid reading, update temperature
    temperature = readTempC == 999 ? temperature : readTempC;

    // Fan speed calculation based on temperature range
    fanSpeed = (int)(((temperature - minTemp) / (maxTemp - minTemp)) * 101);

    // Clamp fan speed to 0–100%
    if (temperature < minTemp) {
      fanSpeed = 0;
    } else if (temperature > maxTemp) {
      fanSpeed = 100;
    }
    if (fanSpeed < 0) {
      fanSpeed = 0;
    } else if (fanSpeed > 100) {
      fanSpeed = 100;
    }

    // Convert percentage to analogWrite PWM value
    fanSpeed = map(fanSpeed, 0, 100, 0, 265);
    fanSpeedVal = (int)(fanSpeed / 12.75 * 5);        // Simplified step resolution
    fanSpeed = (int)(fanSpeedVal / 5 * 12.75);
    fanSpeed = fanSpeed > 255 ? 255 : fanSpeed;       // Limit to max PWM

    analogWrite(fanPin, fanSpeed); // Apply PWM to fan


    // --- Menu Navigation for AUTO Mode ---
    // State transitions and button actions (adjusting temp thresholds, switching data source)
    // State meanings:
    //    State 1: Menu for "Range Config"/"Data Source"; "Range Config" selected
    //    State 2: Menu for "Range Config"->"Max Temp"/"Min Temp"; "Max Temp" selected
    //    State 3: "Max Temp" configuration
    //    State 4: Menu for "Range Config"->"Max Temp"/"Min Temp"; "Min Temp" selected
    //    State 5: "Min Temp" configuration
    //    State 6: Menu for "Range Config"/"Data Source"; "Data Source" selected
    //    State 7: Menu for "Data Source"->"Local Sensor"/"Weather API"; "Local Sensor" selected
    //    State 8: Menu for "Data Source"->"Local Sensor"/"Weather API"; "Weather API" selected

    if (state == 1 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      prevEnterButtonState = LOW;
      prevState = 2;
      state = 2;
      delay(debounceDelay);
    }
    else if (state == 1 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 6;
      state = 6;
      delay(debounceDelay);
    }
    else if (state == 1 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 6;
      state = 6;
      delay(debounceDelay);
    }

    else if (state == 2 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      prevEnterButtonState = LOW;
      prevState = 3;
      state = 3;
      prevSetMaxTemp = 999;
      delay(debounceDelay);
    }
    else if (state == 2 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 4;
      state = 4;
      delay(debounceDelay);
    }
    else if (state == 2 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 4;
      state = 4;
      delay(debounceDelay);
    }

    else if (state == 3 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      maxTemp = setMaxTemp;
      prevEnterButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }
    else if (state == 3 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      setMaxTemp += 0.5;
      if (setMaxTemp > 50) {
        setMaxTemp = 50;
      }
      delay(debounceDelay);
    }
    else if (state == 3 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      setMaxTemp -= 0.5;
      if (setMaxTemp < minTemp) {
        setMaxTemp = minTemp;
      }
      delay(debounceDelay);
    }

    else if (state == 4 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      prevEnterButtonState = LOW;
      prevState = 5;
      state = 5;
      prevSetMinTemp = 999;
      delay(debounceDelay);
    }
    else if (state == 4 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 2;
      state = 2;
      delay(debounceDelay);
    }
    else if (state == 4 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 2;
      state = 2;
      delay(debounceDelay);
    }

    else if (state == 5 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      minTemp = setMinTemp;
      prevEnterButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }
    else if (state == 5 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      setMinTemp += 0.5;
      if (setMinTemp > maxTemp) {
        setMinTemp = maxTemp;
      }
      delay(debounceDelay);
    }
    else if (state == 5 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      setMinTemp -= 0.5;
      if (setMinTemp < 0) {
        setMinTemp = 0;
      }
      delay(debounceDelay);
    }

    else if (state == 6 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      prevEnterButtonState = LOW;
      prevState = 7;
      state = 7;
      delay(debounceDelay);
    }
    else if (state == 6 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }
    else if (state == 6 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }

    else if (state == 7 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      dataSource = false;
      prevEnterButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }
    else if (state == 7 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 8;
      state = 8;
      delay(debounceDelay);
    }
    else if (state == 7 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 8;
      state = 8;
      delay(debounceDelay);
    }
    
    else if (state == 8 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      dataSource = true;
      prevEnterButtonState = LOW;
      prevState = 1;
      state = 1;
      delay(debounceDelay);
    }
    else if (state == 8 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 7;
      state = 7;
      delay(debounceDelay);
    }
    else if (state == 8 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 7;
      state = 7;
      delay(debounceDelay);
    }
  }

  // -------------------- MANUAL MODE --------------------
  if (mode == 1) {
    readVal = readTemperature();
    float readTempC = readVal[0];
    temperature = readTempC == 999 ? temperature : readTempC;

    // Use potentiometer value to manually control fan speed
    int potValue = analogRead(potPin);
    fanSpeed = map(potValue, 0, 1023, 0, 265);
    fanSpeedVal = (int)(fanSpeed / 12.75 * 5);
    fanSpeed = (int)(fanSpeedVal / 5 * 12.75);
    fanSpeed = fanSpeed > 255 ? 255 : fanSpeed;

    analogWrite(fanPin, fanSpeed);


    // --- Menu Navigation for MANUAL Mode ---
    // State transitions and button actions (switching data source)
    // State meanings:
    //    State 0: Menu for "Data Source"; "Data Source" selected
    //    State 9: Menu for "Data Source"->"Local Sensor"/"Weather API"; "Local Sensor" selected
    //    State 10: Menu for "Data Source"->"Local Sensor"/"Weather API"; "Weather API" selected
    
    if (state == 0 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      prevEnterButtonState = LOW;
      prevState = 9;
      state = 9;
      delay(debounceDelay);
    }

    else if (state == 9 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      dataSource = false;
      prevEnterButtonState = LOW;
      prevState = 0;
      state = 0;
      delay(debounceDelay);
    }
    else if (state == 9 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 10;
      state = 10;
      delay(debounceDelay);
    }
    else if (state == 9 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 10;
      state = 10;
      delay(debounceDelay);
    }

    else if (state == 10 && enterButtonState == LOW && prevEnterButtonState == HIGH) {
      dataSource = true;
      prevEnterButtonState = LOW;
      prevState = 0;
      state = 0;
      delay(debounceDelay);
    }
    else if (state == 10 && upButtonState == LOW && prevUpButtonState == HIGH) {
      prevUpButtonState = LOW;
      prevState = 9;
      state = 9;
      delay(debounceDelay);
    }
    else if (state == 10 && downButtonState == LOW && prevDownButtonState == HIGH) {
      prevDownButtonState = LOW;
      prevState = 9;
      state = 9;
      delay(debounceDelay);
    }
  }

  // -------------------- FAN STATUS DISPLAY (Display 1) --------------------

  String modeText = (mode == 0) ? "Auto" : "Manual";
  fanSpeedVal = fanSpeedVal > 100 ? 100 : fanSpeedVal;

  // Transistor on/off based on speed
  if (fanSpeedVal < 5) {
    digitalWrite(transistorPin, LOW);
  } else {
    digitalWrite(transistorPin, HIGH);
  }

  fanSpeedVal = ((int) (fanSpeedVal / 5)) * 5;

  // Fan speed & mode display logic
  if (mode != prevMode || fanSpeedVal != prevFanSpeedVal) {
    prevMode = mode;
    prevFanSpeedVal = fanSpeedVal;

    String fanSpeedText = "Fan Speed: " + String(fanSpeedVal);
    if (fanSpeed < 10) fanSpeedText = "Fan Speed:  " + String(fanSpeedVal);

    if (mode == 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(fanSpeedText);
      lcd.setCursor(0, 1);
      lcd.print("Mode: ");
      lcd.print(modeText);
    } 
    else if (mode == 0 && millis() - fanSpeedUpdate > fanSpeedUpdateDelay && fanSpeedVal != 100 && fanSpeedVal != 0) {
      fanSpeedUpdate = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(fanSpeedText);
      lcd.setCursor(0, 1);
      lcd.print("Mode: ");
      lcd.print(modeText);
    }
    else if (mode == 0 && fanSpeedVal == 100 || fanSpeedVal == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(fanSpeedText);
      lcd.setCursor(0, 1);
      lcd.print("Mode: ");
      lcd.print(modeText);
    }
  }

  // -------------------- AMBIENT TEMPERATURE & HUMIDITY DISPLAY (Display 2) --------------------

  if (millis() - prevTempTime >= tempDelay) {
    prevTempTime = millis();

    Serial.println("Calling readTemperature for display!!!");
    readVal = readTemperature();
    float readTempC = readVal[0];
    int humidity = readVal[2];
    Serial.println("Displaying vals: " + String(readTempC) + " " + String(humidity));
    tempDisplay = readTempC == 999 ? temperature : readTempC;

    if (readTempC == 999 && humidity == 999) {
      lcd2.clear();
      lcd2.setCursor(0, 0);
      lcd2.print("WiFi");
      lcd2.setCursor(0, 1);
      lcd2.print("Disconnected");
    } else {
      lcd2.clear();
      lcd2.setCursor(0, 0);
      lcd2.print("Temp: ");
      lcd2.print(tempDisplay);
      lcd2.print(" C");
      lcd2.setCursor(0, 1);
      lcd2.print("Humidity: ");
      lcd2.print(humidity);
      lcd2.print("%");
    }
  }

  // -------------------- MENU DISPLAY (Display 3) --------------------

  if (state != 3 && state != 5 && state != prevDisplayState) {
    prevDisplayState = state;
    if (state == 0) {
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("> Data Source");
    } 
    else if (state == 1) {
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("> Range Config");
      lcd3.setCursor(0, 1);
      lcd3.print("  Data Source");
    }
    else if (state == 2) {
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("> Max Temp");
      lcd3.setCursor(0, 1);
      lcd3.print("  Min Temp");
    }
    else if (state == 4) {
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("  Max Temp");
      lcd3.setCursor(0, 1);
      lcd3.print("> Min Temp");
    }
    else if (state == 6) {
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("  Range Config");
      lcd3.setCursor(0, 1);
      lcd3.print("> Data Source");
    }
    else if (state == 7) {
      String weatherString = "";
      String localString = "";

      if (dataSource) {
        localString += ">  Local Sensor";
        weatherString += "  *Weather API";
      } else {
        localString = "> *Local Sensor";
        weatherString = "   Weather API";
      }

      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print(localString);
      lcd3.setCursor(0, 1);
      lcd3.print(weatherString);
    }
    else if (state == 8) {
      String weatherString = "";
      String localString = "";
      
      if (dataSource) {
        localString += "   Local Sensor";
        weatherString += "> *Weather API";
      } else {
        localString = "  *Local Sensor";
        weatherString = ">  Weather API";
      }

      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print(localString);
      lcd3.setCursor(0, 1);
      lcd3.print(weatherString);
    }
    else if (state == 9) {
      String weatherString = "";
      String localString = "";

      if (dataSource) {
        localString += ">  Local Sensor";
        weatherString += "  *Weather API";
      } else {
        localString = "> *Local Sensor";
        weatherString = "   Weather API";
      }

      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print(localString);
      lcd3.setCursor(0, 1);
      lcd3.print(weatherString);
    }
    else if (state == 10) {
      String weatherString = "";
      String localString = "";
      
      if (dataSource) {
        localString += "   Local Sensor";
        weatherString += "> *Weather API";
      } else {
        localString = "  *Local Sensor";
        weatherString = ">  Weather API";
      }

      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print(localString);
      lcd3.setCursor(0, 1);
      lcd3.print(weatherString);
    }
  }

  // -------------------- Live Value Adjustment (MAX/MIN Temp) Screens --------------------

  if (state == 3 && setMaxTemp != prevSetMaxTemp) {
    prevSetMaxTemp = setMaxTemp;
    lcd3.clear();
    lcd3.setCursor(0, 0);
    lcd3.print("Max Temp: ");
    lcd3.print(setMaxTemp);
    lcd3.setCursor(0, 1);
    lcd3.print("ENTER to confirm");
  }
  else if (state == 5 && setMinTemp != prevSetMinTemp) {
    prevSetMinTemp = setMinTemp;
    lcd3.clear();
    lcd3.setCursor(0, 0);
    lcd3.print("Min Temp: ");
    lcd3.print(setMinTemp);
    lcd3.setCursor(0, 1);
    lcd3.print("ENTER to confirm");
  }
}
