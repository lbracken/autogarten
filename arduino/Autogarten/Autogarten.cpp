/**
 * autogarten
 *
 * https://github.com/lbracken/autogarten
 * license: MIT, see LICENSE for more details.
 *
 * Library that provides support for creating Arduino based probes that
 * communicate with an Autogarten Control Server over WiFi.  Probe may contain
 * a variety of sensors and actuators.  For more information, see autogarten
 * project page.
 *
 *
 * Requirements
 * ================
 *  [Hardware]
 *    + Arduino UNO Board (rev3)  
 *    + Arduino WiFi Shield (R3)
 *  [Software]
 *    + Arduino 1.0.5+
 *    + Arduino WiFi library [http://arduino.cc/en/Reference/WiFi]
 *    + Time library [http://www.pjrc.com/teensy/td_libs_Time.html]
 *    + TimeAlarms library [http://www.pjrc.com/teensy/td_libs_TimeAlarms.html]
 *    + OneWire library [http://www.pjrc.com/teensy/td_libs_OneWire.html]
 *
 *
 * Important Notes
 * ================
 *  + Be sure to update the firmware on the Arduino WiFi Shield
 *
 *  + Assumes WiFi connection is to a WPA/WPA2 protected network.
 *
 *  + The Arduino WiFi shield uses the SPI bus and other digital pins.
 *      Do not use pins 7, 10, 11, 12 or 13 in your sketch as they are in use
 *      For more details: http://arduino.cc/en/Main/ArduinoWiFiShield
 *
 *  + This probe supports devices that use the OneWire (1-Wire) protocol.
 *    Pin 9 has been reserved for all OneWire protocol communication.
 *    To change, see ONE_WIRE_PIN
 *
 *  + If using multiple OneWire devices, the order in which they are added
 *    matters. Call addSensor() in increasing order according to the device's
 *    unique 64-bit ROM code.
 *
 *  + Memory is tight. By default a maximum of 5 sensors is allowed.
 *    To change, see MAX_SENSORS
 * 
 *  + For best results, consider adjusting the size of MAX_DATA_POINTS.  This
 *    determines how many sensor read data points can be stored in between
 *    syncs with the control server.  For example, if you plan to read sensor
 *    values every 15 mins and sync with the control server every 4hrs, then 
 *    you'd need to store at least 16 data points.  Consider rounding up to 
 *    account for any connection issues that might delay a sync with the
 *    control server.  One the max data points has been reached, the oldest 
 *    data will be overwritten with new data (works like a rolling buffer).
 *    
 *
 * Error Codes
 * ================
 *  100  Using a reserved pin.  See 'Important Notes' for more details.
 *  101  Max sensors added.  See 'Important Notes' for more details.
 *  102  Using reserved OneWire pin, but sensor is not of type OneWire.
 *  103  Unknown sensor type
 *  110  Must specify a valid analog pin
 *  111  Must specify a valid digital pin
 *  112  Must specify the preset OneWire pin
 *  200  No Onewire device with sensorId found
 *  201  Invalid CRC for OneWire device
 *  202  Unsupported OneWire device type
 *  300  Unable to connect to control server
 *  301  Bad response from control server
 *
 */
#include <Arduino.h>
#include <Autogarten.h>


OneWire _oneWire(ONE_WIRE_PIN);


/**
 * Construct a new Autogarten object
 *
 *  + Unique Id for this probe
 */
Autogarten::Autogarten(char probeId[]) {
  _probeId = probeId;
  
  _sensorCount = 0;
  _oneWireDeviceCount = 0;
  _currDataPoints = 0;
}


/**
 * Setup configuration of the probe's Control Server
 *
 *  + Control Server Address
 *  + Control Server Port
 *  + Control Server Security Token
 */
void Autogarten::setupControlServer(char address[], int port, char token[]) {
  _ctrlSrvrAddr  = address;
  _ctrlSrvrPort  = port;
  _ctrlSrvrToken = token;
  _ctrlSrvrSyncCount = 0;
}


/**
 * Setup configuration and behavior of the WiFi connection
 *
 *  + WiFi network SSID
 *  + WiFi network WPA/2 Password
 *  + True to keep WiFi connection continuously open.  False to connect/disconnect on demand
 */
void Autogarten::setupWiFi(char ssid[], char password[], boolean keepAlive) {
  _wifiSSID = ssid;
  _wifiPassword = password;
  _wifiKeepAlive = keepAlive;
  //_wifiStatus = WiFi.WL_IDLE_STATUS;
  
  // If keeping the connection alive, then go ahead and connect.
  // Otherwise connect and disconnect as needed.
  if (_wifiKeepAlive) {
    connectToWiFi();
  }
}


/**
 * Sync with the control server.  This sends collected sensor data as well as
 * gets new instructions from the control server.
 *
 *  + Function to pass to TimeAlarms for next control server sync
 *  + Function to pass to TimeAlarms for reading sensors
 */
void Autogarten::syncWithControlServer(void (*onSyncWithControlServer)(), void (*onReadSensors)()) {

  // Ensure we have a WiFi connection established
  connectToWiFi();

  byte connectionAttempts = 0;
  while (connectionAttempts <= MAX_CONNECTION_ATTEMPTS) {
    connectionAttempts++;

    // TODO: Is this required, is there a better way?
    _wifiClient.stop();  
    _wifiClient.flush();
   
    // Log the attempt to connect...
    Serial.println();
    Serial.print(F("Connecting to "));
    Serial.print(_ctrlSrvrAddr);
    Serial.print(F(":"));
    Serial.print(_ctrlSrvrPort);
    Serial.print(F(" attempt "));
    Serial.print(connectionAttempts);
    Serial.print(F("  Sync Count:"));
    Serial.println(_ctrlSrvrSyncCount);

    // Connect to control server
    if (_wifiClient.connect(_ctrlSrvrAddr, _ctrlSrvrPort)) {
      Serial.println(F("Synchronizing..."));

      // Setup HTTP Request Headers
      _wifiClient.println(F("POST /probe_sync HTTP/1.1"));    
      _wifiClient.println(F("User-Agent: Arduino/1.0"));
      _wifiClient.println(F("Connection: close"));
      _wifiClient.println(F("Content-Type: application/json"));

      // Set request body and send request
      String requestBody = createProbeSyncRequest(connectionAttempts);
      _wifiClient.print(F("Content-Length: "));
      _wifiClient.println(requestBody.length());
      _wifiClient.println();
      _wifiClient.println(requestBody);

      // Wait for response from server...
      // TODO: There has to be a better way than just wait n seconds...
      delay(5000);

      // Read the first line of the HTTP Response.  It should contain the 
      // HTTP Response Code.  Only continue if it's 200 OK. (LineFeed=10)
      String response = _wifiClient.readStringUntil(10);
      if (response.indexOf("200 OK") < 0) {      	
        Serial.println(response);
      	printErrorCode(301);
        return;        
      }

      // Read/skipover the remaining HTTP Headers.  The HTTP Response
      // body is reached when we encounter a double LineFeed.
      do {
        response = _wifiClient.readStringUntil(10);
        //Serial.println(response);     
      } while (response.length() > 1);

      // Read HTTP Response Body
      response = _wifiClient.readString();
      Serial.println(response);

      // Resync clock with the control server (approximately)
      long currTime = getValueFromJSON(response, "curr_time").toInt();
      setTime(currTime);

      // Schedule the next sync with the control server
      long nextSync = getValueFromJSON(response, "next_sync").toInt() - currTime;
      Alarm.timerOnce(nextSync, onSyncWithControlServer);

      // Schedule sensor reads...
      int sensorFrequency = getValueFromJSON(response, "sensor_freq").toInt();
      Alarm.timerRepeat(sensorFrequency, onReadSensors);

      Serial.println(F("... sync complete"));
      _ctrlSrvrSyncCount++;

      // If not keeping the WiFi connection alive, we can now disconnect
      if (!_wifiKeepAlive) {
      	disconnectFromWiFi();
      }    
            
      return;
    }
  }

  printErrorCode(300);
  Serial.println(F("Failure"));
}


/**
 * Adds the given sensor to the probe
 *
 *  + Unique Id for the sensor
 *  + Pin this sensor is attached to.  0-13 or A0-A5.  See reserved pins.
 *  + Type of this sensor.  Select from defined constants SENSOR_TYPE_*
 */
boolean Autogarten::addSensor(char sensorId[], int pin, int type) {
  
  // Verify a reserved PIN isn't being used... 
  if (pin == 7 || pin == 10 || pin == 11 || pin == 12 || pin == 13) {
    printErrorCode(100);
    return false;
  }
  
  // Verify we aren't exceeded the maximum number of supported sensors
  if (_sensorCount >=  MAX_SENSORS) {
    printErrorCode(101);
    return false; 
  }

  // If OneWire pin is used, ensure that type matches
  if (pin == ONE_WIRE_PIN && type != SENSOR_TYPE_ONEWIRE_TEMP) {
    printErrorCode(102);
    return false; 
  }

  // Setup according to sensor type
  byte oneWireIdx = 0;
  switch (type) {

    case SENSOR_TYPE_ANALOG:
      if (!isAnalogPin(pin)) {
        printErrorCode(110);
        return false;
      }       
      break;
      
    case SENSOR_TYPE_DIGITAL:
      if (!isDigitalPin(pin)) {
        printErrorCode(111);
        return false;
      }
      pinMode(pin, INPUT);      
      break;      

    case SENSOR_TYPE_ONEWIRE_TEMP:
      if (pin != ONE_WIRE_PIN) {
        printErrorCode(112);
        return false;
      }   
      
      oneWireIdx = _oneWireDeviceCount++;     
      break;

    default:
        printErrorCode(103);
        return false;
  }

  // Add the sensor
  Sensor sensor = (Sensor){sensorId, pin, type, oneWireIdx};
  _sensors[_sensorCount++] = sensor;
  
  Serial.print(F(" + "));
  Serial.print(sensor.id);
  Serial.print(F(" on "));
  Serial.println(sensor.pin);
  return true;
}


/**
 * Read value from an analog sensor.
 */ 
int Autogarten::readAnalogSensor(char sensorId[]) {
  byte pin = getPinForSensorId(sensorId);
  int value = analogRead(pin);
  
  printSensorResult(sensorId, pin, value);
  return value;
}


/**
 * Read a value from an digital sensor.
 */ 
byte Autogarten::readDigitalSensor(char sensorId[]) {
  byte pin = getPinForSensorId(sensorId);
  byte value = digitalRead(pin);
  
  printSensorResult(sensorId, pin, value);
  return value;
}


/**
 * Read a temperature value from a OneWire device
 *
 * Important Notes
 * ==================
 * This function has been streamlined to ONLY support a DS18S20 digital
 * thermometer running in non-parasitic power mode. If using another OneWire
 * device then changes are required. See the Arduino OneWire page for details
 * and code. 
 *
 * Note that multiple DS18S20 devices can be wired together on the same bus.
 * A 4.7K resistor between VCC and Data.
 */
 float Autogarten::readOneWireTempSensor(char sensorId[]) {

  byte oneWireIdx = getOneWireIdxForSensorId(sensorId);
  byte deviceCount = 0;
  byte addr[8];  
  
  // Iterate over all devices on the OneWire bus
  _oneWire.reset_search();
  while (_oneWire.search(addr) ){
    delay(250);

    // If this isn't the device we're looking for, continue.
    if (oneWireIdx != deviceCount++) {
    	continue;
    } 
    
    // Print the device's unique 64-bit ROM code
    /*
    Serial.print(" * R=");
    for( i = 0; i < 8; i++) {
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    Serial.println("---");
    */
    
    // Verify valid CRC, if not return error value
    if (OneWire::crc8( addr, 7) != addr[7]) {
      printErrorCode(201);
      return 0.0;
    }
    
    // Verify is chip is a DS18B20, if not return error value.
    // The first ROM byte indicates which chip (0x28 = DS18B20).
    if(addr[0] != 0x28) {
      printErrorCode(202);
      return 0.0;
    }
    
    // Reset the OneWire bus, select a device then start a conversion
    _oneWire.reset();
    _oneWire.select(addr);
    _oneWire.write(0x44);  
    delay(800);
    
    // Reset the OneWire bus, select a device then read scratchpad
    _oneWire.reset();
    _oneWire.select(addr);    
    _oneWire.write(0xBE);

    // Read the data on the bus (9 bytes)
    byte data[12];
    for (byte i = 0; i < 9; i++) {
      data[i] = _oneWire.read();
    }
    
    // Convert the data (hex) to a temperature value.
    // Since we're only supporting a DS18S20 device, we can assume
    // 12 bit resolution (750ms conversion time).
    int16_t raw = (data[1] << 8) | data[0];
    float value = convertCelsiusToFahrenheit((float)raw / 16.0);        
    printSensorResult(sensorId, ONE_WIRE_PIN, value);    
    return value;
  }

  printErrorCode(200);
  return 0.0;
}


/**
 * Reads and stores the value for all sensors
 */
void Autogarten::readSensors() {

  if (PRINT_SENSOR_READINGS) {
    Serial.println();
  }

  for (byte ctr=0; ctr < _sensorCount; ctr++) {
    Sensor sensor = _sensors[ctr];
    int value = 0;

    switch (sensor.type) {
      case SENSOR_TYPE_ANALOG:
        value = readAnalogSensor(sensor.id);
        break;

      case SENSOR_TYPE_DIGITAL:
        value = readDigitalSensor(sensor.id);
        break;

      case SENSOR_TYPE_ONEWIRE_TEMP:
        float fValue = readOneWireTempSensor(sensor.id);
        // To save space, convert to int.  Multiply to keep precision.
        value = (int)(fValue * 100);
        break;
    }

    unsigned int timestampDelta = 100;	// TODO
    _sensors[ctr].dataPoints[_currDataPoints] = (DataPoint){timestampDelta, value};
  }

  _currDataPoints++;
  if(_currDataPoints >= MAX_DATA_POINTS) {
  	_currDataPoints = 0;
  }
}


/**
 * Prints to serial output detailed debug information
 */
void Autogarten::printDebugInfo() {

  Serial.println(F("---------------------------------"));
  for (byte ctr=0; ctr < _sensorCount; ctr++) {
    Sensor sensor = _sensors[ctr];
    Serial.print(F("  "));
    Serial.println(sensor.id);

    for (byte xtr=0; xtr < _currDataPoints; xtr++) {
      Serial.print(F("  "));
      Serial.print(sensor.dataPoints[xtr].timestampDelta);
      Serial.print(F(" : "));
      Serial.println(sensor.dataPoints[xtr].value);
    }
  }
}


/**
 * Prints to serial output the amount of free memory
 */
void Autogarten::printFreeMemory() {
  Serial.print(F("Free Memory:"));
  Serial.println(getFreeMemory());
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                                                                           */
/*                            PRIVATE FUNCTIONS                              */
/*                                                                           */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


// Construct the body of a probe sync request to the control server
String Autogarten::createProbeSyncRequest(byte connectionAttempts) {

  String syncRequest = "{\"probe_id\":\"";
  syncRequest += _probeId;
  syncRequest += "\",";
  syncRequest += "\"token\":\"";
  syncRequest += _ctrlSrvrToken;
  syncRequest += "\",";
  syncRequest += "\"sync_count\":";
  syncRequest += _ctrlSrvrSyncCount;
  syncRequest += ",";
  syncRequest += "\"curr_time\":";
  syncRequest += timeStatus() ? now() : 0;
  syncRequest += ",";
  syncRequest += "\"connection_attempts\":";
  syncRequest += connectionAttempts;
  syncRequest += ",";
  syncRequest += "\"sensor_data\":[]";	// TODO
  syncRequest += "}";
  
  return syncRequest;
}


// Establish a WiFi connection. If already connected, no action is taken.
void Autogarten::connectToWiFi() {

  // If already connected, then we're done..
  if (_wifiStatus == WL_CONNECTED) {
    return;
  }
  
  byte connectionAttempts = 0;
  while (_wifiStatus != WL_CONNECTED) {
    
    // Increment, then verify within allowable connection attempts...
    connectionAttempts++;
    if (connectionAttempts > MAX_CONNECTION_ATTEMPTS) {
      Serial.println(F("Failure"));
      return; 
    }    

    // Log the attempt to connect...
    Serial.print(F("Connecting to "));
    Serial.print(_wifiSSID);
    Serial.print(F(" attempt "));
    Serial.println(connectionAttempts);
    
    // Connect to WiFi, wait a few seconds for connection to establish...
    // Currently using DHCP settings from WiFi router. To change, see about
    // calling WiFi.config()
    _wifiStatus = WiFi.begin(_wifiSSID, _wifiPassword);
    delay(10000);
  }
  
  // Once connected, print connection information
  Serial.print(F("Success"));
  Serial.print(F(" IP:"));
  Serial.print(WiFi.localIP());
  Serial.print(F(" RSSI:"));
  Serial.println(WiFi.RSSI());
}


// Disconnect from any WiFi connection.  If not connected, no action is taken.
void Autogarten::disconnectFromWiFi() {
  WiFi.disconnect();
  _wifiStatus = WL_IDLE_STATUS;
}


// Returns the pin for the given sensorId
byte Autogarten::getPinForSensorId(char sensorId[]) {
  for (byte ctr=0; ctr < _sensorCount; ctr++) {
    if (_sensors[ctr].id == sensorId) {
      return _sensors[ctr].pin;
    }    
  }  
  return -1;  
}


// Returns the OneWireIdx for the given sensorId
byte Autogarten::getOneWireIdxForSensorId(char sensorId[]) {
  for (byte ctr=0; ctr < _sensorCount; ctr++) {
    if (_sensors[ctr].id == sensorId) {
      return _sensors[ctr].oneWireIdx;
    }    
  }  
  return 0;  
}


// Returns true if given pin is a digital pin
boolean Autogarten::isDigitalPin(int pin) {
  return pin >= 0 && pin <= 13;
}


// Retrurns true if given pin is an analog pin
boolean Autogarten::isAnalogPin(int pin) {
  return pin >= A0 && pin <= A5;
}


// For debugging, prints results of a sensor read
void Autogarten::printSensorResult(char sensorId[], int pin, float value) {

  if (!PRINT_SENSOR_READINGS) {
    return;
  }

  Serial.print(sensorId);
  Serial.print(F(":"));
  Serial.print(pin);
  Serial.print(F(" = "));
  Serial.println(value);  
}


// Convert a celsius value to Fahrenheit, because Amurika
float Autogarten::convertCelsiusToFahrenheit(float degreesCelsius) {
  return degreesCelsius * 1.8 + 32.0;
}


// Very primitive method for parsing JSON strings
String Autogarten::getValueFromJSON(String jsonString, String jsonKey) {

  // Using a proper JSON parsing library would be ideal.  However, the JSON
  // parsing needed for this project is very basic, and the following libraries
  // used a far bit of memory in testing.  However, it would be worth taking 
  // another look at the following:
  //  + https://github.com/not404/json-arduino  (No support for nested objects)
  //  + https://github.com/interactive-matter/aJson
  //  + https://github.com/bblanchon/ArduinoJsonParser
  
  int startIdx = jsonString.indexOf(jsonKey);  
  if (startIdx < 0) {
    return ""; 
  }
  
  int endIdx = jsonString.indexOf(",", startIdx);
  if (endIdx < 0) {
    endIdx = jsonString.length() - 1;
  } 
  
  String toReturn = jsonString.substring(startIdx+jsonKey.length(), endIdx);
  toReturn.replace('"', ' ');
  toReturn.replace(':', ' ');
  toReturn.trim(); 
  return toReturn;
}


// Prints out an error code
void Autogarten::printErrorCode(int errorCode) {
  Serial.print(F("ERROR "));
  Serial.println(errorCode);
}


// Returns bytes of free RAM.  See 'Arduino Cookbook' page 587
extern int __bss_end;
extern void *__brkval;
int Autogarten::getFreeMemory() {
  int freeValue;
  if((int)__brkval == 0)
    freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else
    freeValue = ((int)&freeValue) - ((int)__brkval);
  return freeValue; 
}