/**
 * autogarten - arduino based probe
 *
 *  Base sketch for a probe that communicates with a control server over WiFi.
 *  Assumes that connection is to a WPA/WPA2 protected network.
 *
 *  Requirements:
 *    * Arduino UNO Board (rev3)
 *    * Arduino WIFI Shield (R3)
 *    * Arduino v1.0.4+ (tested with 1.0.5)
 *    * Arduino WiFi library
 *    * OneWire library: http://playground.arduino.cc/Learning/OneWire
 *
 *
 *  !!! Important -- Reserved PINs !!!
 *
 *  + The Arduino WiFi shield uses the SPI bus and other digital pins.
 *      Do not use pins 7, 10, 11, 12 or 13 in your sketch as they are in use
 *      For more details: http://arduino.cc/en/Main/ArduinoWiFiShield
 *
 *  + Currently pin 9 is reserved for all OneWire protocol communication.
 *      See local functions that use OneWire for more details:
 *      - readOneWireTempSensor()
 *
 *
 * Error Codes:
 *
 *  Code   Message
 *  ----   ------------------------------------------------
 *   100   Trying to use a reserved pin.
 *   101   Must specify a valid analog pin.
 *   102   Must specify a valid digital pin.
 *   103   Must specify the OneWire pin.
 *   104   Too many OneWire devices. Increase maxOneWireDevices.
 *   105   Invalid sensor type   
 *   200   No 1-wire devices found
 *   201   Invalid CRC
 *   202   Unsupported 1-wire device type
 *
 *
 * https://github.com/lbracken/autogarten
 * license: MIT, see LICENSE for more details.
 */
#include <WiFi.h>
#include <OneWire.h>

const int SENSOR_TYPE_ANALOG_GENERIC = 1;
const int SENSOR_TYPE_DIGITAL_GENERIC = 2;
const int SENSOR_TYPE_ONEWIRE_TEMP = 3;

const int ERROR_VALUE = -1000;

const int oneWirePin = 9;
const int maxOneWireDevices = 3;
int oneWireDeviceCount = 0;
OneWire oneWire(9);

/* WiFi variables */
const int wifiConnectionWait = 10000;  // In ms
const int maxWiFiConnectionAttempts = 10;
WiFiClient wifiClient;
int wifiStatus = WL_IDLE_STATUS;
int wifiStatusLEDPin;
boolean wifiPersistConnection;
char *wifiSSID;
char *wifiPassword;


/* Probe and Control Server variables */
const int maxControlServerConnectionAttempts = 10;
char *probeId;
char *controlServerAddress;
int  controlServerPort;
char *controlServerToken;
int  controlServerSyncCount = 1;

/* Sensor indices */
char *sensorIds[19];
char *oneWireSensorIds[maxOneWireDevices];




void setup() {
  Serial.begin(9600);
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Starting autogarten probe");
  
  // Initialize the probe and wifi configuration
  initProbe("autogarten_probe", "-----", 5000, "changeme");
  initWiFi("-----", "-----", false);
  
  // Add any sensors
  addSensor("light_level_1", A0, SENSOR_TYPE_ANALOG_GENERIC);
  addSensor("light_level_2", A1, SENSOR_TYPE_ANALOG_GENERIC);  
  addSensor("switch_1", 2, SENSOR_TYPE_DIGITAL_GENERIC);
  addSensor("soil_temp", oneWirePin, SENSOR_TYPE_ONEWIRE_TEMP);
  addSensor("air_temp", oneWirePin, SENSOR_TYPE_ONEWIRE_TEMP);
  
  // Add any actuators
  // TODO...  
}


void loop() {   
  delay(5000);
  
  Serial.println();
  Serial.print("Free Memory: ");
  Serial.println(memoryFree());
  
  readAnalogGenericSensor("light_level_1");
  readAnalogGenericSensor("light_level_2");
  readDigitalGenericSensor("switch_1");
  readOneWireTempSensor("soil_temp");
  readOneWireTempSensor("air_temp");
  
  //syncWithControlServer();
}




/**
 * Initialize settings and behavior for the probe
 *
 *   * Id of Probe
 *   * Control Server Address
 *   * Control Server Port
 *   * Control Server Security Token
 */
void initProbe(char *id, char *address, int port, char *token){
  probeId = id;
  controlServerAddress = address;
  controlServerPort = port;
  controlServerToken = token;
}


/**
 * Initialize settings and behavior for WiFi connections
 *   * WiFi network SSID
 *   * WiFi network WPA/2 Password
 *   * True to persist WiFi connection.  False to connection/disconnect on demand
 */
void initWiFi(char *ssid, char *password, boolean persistConnection) {
  wifiSSID = ssid;
  wifiPassword = password;
  wifiPersistConnection = persistConnection;
  
  // If keeping the connection open, then go ahead and connect.
  // Otherwise connection and disconnect as needed.
  if (persistConnection) {
    connectToWiFi();
  }
}


/**
 * If not already connected, establish a WiFi connection
 */
void connectToWiFi() {
  
  // If already connected, then we're done..
  if (wifiStatus == WL_CONNECTED) {
    return;
  }  
  
  int connectionAttempts = 0;
  while (wifiStatus != WL_CONNECTED) {
    
    // Increment, then verify within allowable connection attempts...
    connectionAttempts++;
    if (connectionAttempts > maxWiFiConnectionAttempts) {
      Serial.println("... connection failure.  Max attemps reached.");
      return; 
    }    

    // Log the attempt to connect...
    Serial.print("Connecting to WiFi network '");
    Serial.print(wifiSSID);
    Serial.print("'.  Attempt #");
    Serial.print(connectionAttempts);
    Serial.println("...");
    
    // Connect to WiFi, wait a few seconds for connection to establish...
    wifiStatus = WiFi.begin(wifiSSID, wifiPassword);
    delay(wifiConnectionWait);
  }
  
  // Once connected, print connection information
  Serial.print("... connection successful IP:");
  Serial.print(WiFi.localIP());
  Serial.print(" RSSI:");
  Serial.println(WiFi.RSSI());
}


/**
 * Disconnect from any WiFi connection.
 * If not connected no action is taken.
 */
void disconnectFromWiFi() {
  WiFi.disconnect();
  wifiStatus = WL_IDLE_STATUS;
}


/**
 * Do a probe sync with the connection server
 */
void syncWithControlServer() {

  // Ensure we have a WiFi connection established
  connectToWiFi();  

  // TODO: Verify this is required and correct
  // Ensure we can start a clean connection to the server  
  wifiClient.stop();
  wifiClient.flush();
  
  int connectionAttempts = 0;
  while (connectionAttempts <= maxControlServerConnectionAttempts) {
    connectionAttempts++;
    
    // Log the attempt to connect...
    Serial.print("Connecting to Control Server [");
    Serial.print(controlServerAddress);
    Serial.print(":");
    Serial.print(controlServerPort);
    Serial.print("].  Attempt #");
    Serial.print(connectionAttempts);
    Serial.println("...");
    
    // Connect to control server
    if (wifiClient.connect(controlServerAddress, controlServerPort)) {
      Serial.println("... connected to Control Server");
      String requestBody = createProbeSyncRequest(connectionAttempts);
      Serial.println(requestBody);
      
      // Setup HTTP Request Headers
      wifiClient.println("POST /probe_sync HTTP/1.1");    
      wifiClient.println("User-Agent: Arduino/1.0");
      wifiClient.println("Connection: close");
      wifiClient.println("Content-Type: application/json");
      wifiClient.print("Content-Length: ");
      wifiClient.println(requestBody.length());
      wifiClient.println();
    
      // Set request body and send request
      wifiClient.println(requestBody);
      Serial.println("... sync complete");
 
      // TODO: If WiFi persistent connections are off, then we should disconnect here once request has been sent.     
            
      controlServerSyncCount++; 
      return;
    }
  }
  
  Serial.println("... failed to connect to Control Server.  Max attemps reached."); 
}

/**
 * Construct the body of a probe sync request to the control server
 */
String createProbeSyncRequest(int connectionAttempts) {
  
  // TODO: Start using a proper JSON library here.  Some options could include..
  //  * https://github.com/not404/json-arduino
  //  * https://github.com/interactive-matter/aJson
  
  String syncRequest = "{\"probe_id\": \"";
  syncRequest += probeId;
  syncRequest += "\", ";
  syncRequest += "\"token\": \"";
  syncRequest += controlServerToken;
  syncRequest += "\", ";
  syncRequest += "\"sync_count\": ";
  syncRequest += controlServerSyncCount;
  syncRequest += ", ";
  syncRequest += "\"curr_time\": ";
  syncRequest += 1399838400;
  syncRequest += ", ";  // TODO: Set current time
  syncRequest += "\"connection_attempts\": ";
  syncRequest += connectionAttempts;
  syncRequest += ", ";
  syncRequest += "\"sensor_data\": []";
  syncRequest += "}";
  
  return syncRequest;
}


/*
 * Adds the given sensor to this probe
 */ 
boolean addSensor(char *sensorId, int pin, int sensorType) {
  
  // TODO: Verify a reserved PIN isn't being used... 

  switch (sensorType) {
    
    case SENSOR_TYPE_ANALOG_GENERIC:
      if (!isAnalogPin(pin)) {        
        Serial.print(sensorId);
        Serial.println(" ERROR 101");
        return false;
      }       
      break;
      
    case SENSOR_TYPE_DIGITAL_GENERIC:
      if (!isDigitalPin(pin)) {
        Serial.print(sensorId);
        Serial.println(" ERROR 102");
        return false;
      }
      pinMode(pin, INPUT);      
      break;      

    case SENSOR_TYPE_ONEWIRE_TEMP:
      if (pin != oneWirePin) {
        Serial.print(sensorId);
        Serial.println(" ERROR 103");
        return false;
      }
      
      if (oneWireDeviceCount >= maxOneWireDevices) {
        Serial.println(" ERROR 104");
        return false;
      }      
      
      // 1-wire devices share a pin, so set the sensorId in the 1-wire array      
      oneWireSensorIds[oneWireDeviceCount++] = sensorId;
      break;

    default:
        Serial.print(sensorId);
        Serial.println(" ERROR 105"); 
  }

  sensorIds[pin] = sensorId;  
  Serial.print(sensorIds[pin]);
  Serial.print(" added on pin ");
  Serial.println(pin);  
  return true;
}


/**
 * Returns the pin for the given sensorId
 */
int getPinForSensorId(char *sensorId) {
  int ctr;
  for (ctr=0; ctr < sizeof(sensorIds)/sizeof(char); ctr++) {
    if (sensorIds[ctr] == sensorId) {
      return ctr;
    }
  }
  return -1;  
}


/**
 * Returns true if pin is a digital pin
 */
boolean isDigitalPin(int pin) {
  return pin >= 0 && pin <= 13;
}


/**
 * Retrurns true if pin is an analog pin
 */
boolean isAnalogPin(int pin) {
  return pin >= A0 && pin <= A5;
}


/**
 * Read a value from an analog sensor.  Examples include...
 *  + Photoresistors for measuring light levels.
 *  + ...
 */ 
float readAnalogGenericSensor(char *sensorId) {
  int pin = getPinForSensorId(sensorId);
  float value = analogRead(pin);
  
  printSensorResult(sensorId, pin, value);
  return value;
}


/**
 * Read a value from an digital sensor.  Examples include...
 *  + Switches
 *  + ...
 */ 
int readDigitalGenericSensor(char *sensorId) {
  int pin = getPinForSensorId(sensorId);
  int value = digitalRead(pin);
  
  printSensorResult(sensorId, pin, value);
  return value;
}


/*
 * Using the OneWire library, reads temperature from a 1-wire device.
 * See: http://playground.arduino.cc/Learning/OneWire
 *
 * !! Important !!
 *     This function has been streamlined to ONLY support a DS18S20 digital thermometer
 *     running in non-parasitic power mode.  If using another 1-wire device then changes
 *     are required. See the Arduino OneWire page for details and code. 
 *
 *     Note that multiple DS18S20 devices can be wired together on the same bus.
 *     Be sure to place a 4.7K resistor between VCC and Data.
 *
 *     When using multiple DS18S20 devices, the order that the addSensor() is called
 *     matters.  Call addSensor() for the first item on the bus.
 */
float readOneWireTempSensor(char *sensorId) {
  
  int deviceCount = 0;
  byte addr[8];  
  
  // Iterate over all devices on the OneWire bus
  oneWire.reset_search();
  while (oneWire.search(addr) ){
    delay(250);
    
    // If this isn't the device we're looking for, continue.
    if (sensorId != oneWireSensorIds[deviceCount++]) {
      continue;
    }    
    
    // Print the device's unique 64-bit ROM code
    /*Serial.print(" * R=");
    for( i = 0; i < 8; i++) {
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    Serial.println("---");   
    */
    
    // Verify valid CRC, if not return error value
    if (OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("ERROR 201");
      return ERROR_VALUE;
    }
    
    // Verify is chip is a DS18B20, if not return error value.
    // The first ROM byte indicates which chip (0x28 = DS18B20).
    if(addr[0] != 0x28) {
      Serial.println("ERROR 202");
      return ERROR_VALUE;
    }
    
    // Reset the 1-Wire bus, select a device then start a conversion
    oneWire.reset();
    oneWire.select(addr);
    oneWire.write(0x44);  
    delay(800);
    
    // Reset the 1-Wire bus, select a device then read scratchpad
    oneWire.reset();
    oneWire.select(addr);    
    oneWire.write(0xBE);

    // Read the data on the bus (9 bytes)
    byte data[12];
    for (int i = 0; i < 9; i++) {
      data[i] = oneWire.read();
    }
    
    // Convert the data (hex) to a temperature value.
    // Since we're only supporting a DS18S20 device, we can assume
    // 12 bit resolution (750ms conversion time).
    int16_t raw = (data[1] << 8) | data[0];
    float value = convertCelsiusToFahrenheit((float)raw / 16.0);        
    printSensorResult(sensorId, oneWirePin, value);    
    return value;
  }

  Serial.println("ERROR 200");
  return ERROR_VALUE;
}


/**
 * For debugging purposes, prints out the results of reading a sensor
 */
void printSensorResult(char *sensorId, int pin, float value) {
  Serial.print(sensorId);
  Serial.print(" on ");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println(value);  
}


/**
 * Convert a celsius value to Fahrenheit, because Amurika
 */
float convertCelsiusToFahrenheit(float degreesCelsius) {
  return degreesCelsius * 1.8 + 32.0;
}


/**
 * Determine amount of free RAM
 * From Arduino Cookbook: Page 587
 */
extern int __bss_end;
extern void *__brkval;
int memoryFree() {
  int freeValue;
  if((int)__brkval == 0)
    freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else
    freeValue = ((int)&freeValue) - ((int)__brkval);
  return freeValue; 
}

