/**
 * autogarten
 *
 * https://github.com/lbracken/autogarten
 * license: MIT, see LICENSE for more details.
 */
#ifndef Autogarten_h
#define Autogarten_h

#include <Arduino.h>
#include <OneWire.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <WiFi.h>

const byte SENSOR_TYPE_ANALOG = 1;
const byte SENSOR_TYPE_DIGITAL = 2;
const byte SENSOR_TYPE_ONEWIRE_TEMP = 3;

const byte MAX_SENSORS = 5;
const byte MAX_DATA_POINTS = 20;  // (Per Sensor)

const byte ONE_WIRE_PIN = 9;
const byte MAX_CONNECTION_ATTEMPTS = 25;
const boolean PRINT_SENSOR_READINGS = true;


struct DataPoint {
  // To minimize memory used, store timestamp delta
  // from _timestampBase.
  unsigned int timestampDelta;
  int value;
};


struct Sensor {
  char *id;
  byte pin;
  byte type;  
  byte oneWireIdx;
  DataPoint dataPoints[MAX_DATA_POINTS];
};


class Autogarten {
  private:
    char *_probeId;

    /* Control Server */
    char *_ctrlSrvrAddr;
    int   _ctrlSrvrPort;
    char *_ctrlSrvrToken;
    int   _ctrlSrvrSyncCount;
    String createProbeSyncRequest(byte connectionAttempts);

    /* WiFI */
    char      *_wifiSSID;
    char      *_wifiPassword;
    boolean    _wifiKeepAlive;
    byte       _wifiStatus;
    WiFiClient _wifiClient;
    void connectToWiFi();
    void disconnectFromWiFi();

    /* Sensors */
    Sensor _sensors[MAX_SENSORS];
    byte _sensorCount;	
    byte _oneWireDeviceCount;	
    byte _currDataPoints;
    long _timestampBase;
    byte getPinForSensorId(char sensorId[]);
    byte getOneWireIdxForSensorId(char sensorId[]);
    boolean isDigitalPin(int pin);
    boolean isAnalogPin(int pin);
    void printSensorResult(char sensorId[], int pin, float value);

    /* Util - Debug Methods */
    float convertCelsiusToFahrenheit(float degreesCelsius);
    String getValueFromJSON(String jsonString, String jsonKey);
    void printErrorCode(int errorCode);
    int getFreeMemory();


  public:
    /* Setup */
    Autogarten(char probeId[]);
    void setupControlServer(char address[], int port, char token[]);
    void setupWiFi(char ssid[], char password[], boolean keepAlive);

    /* Control Server */
    void syncWithControlServer(void (*onSyncWithControlServer)(), void (*onReadSensors)());

    /* Sensors */
    boolean addSensor(char sensorId[], int pin, int type);
    int readAnalogSensor(char sensorId[]);
    byte readDigitalSensor(char sensorId[]);
    float readOneWireTempSensor(char sensorId[]);	
    void readSensors();

    /* Util - Debug Methods */
    void printDebugInfo();
    void printFreeMemory();
};

#endif