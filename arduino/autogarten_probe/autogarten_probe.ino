/**
 * autogarten - arduino based probe
 *
 * This sketch implements an example probe using the Autogarten Arduino library
 * 
 * https://github.com/lbracken/autogarten
 * license: MIT, see LICENSE for more details.
 */
#include <Autogarten.h>
#include <OneWire.h>
#include <WiFi.h>
#include <Time.h>
#include <TimeAlarms.h>

// Define the probe, setting the unique probe id
Autogarten autogarten("autogarten_probe");

void setup(){
  Serial.begin(9600); 
  autogarten.printFreeMemory();
  
  // Setup configuration for Control Server and WiFi connection
  //autogarten.setupControlServer("hostname/ip", 5000, "changeme", 30, 121);
  //autogarten.setupWiFi("SSID", "password", true);  
  
  // Add sensors
  autogarten.addSensor("light-1",  A0, SENSOR_TYPE_ANALOG);
  autogarten.addSensor("light-2",  A1, SENSOR_TYPE_ANALOG);
  autogarten.addSensor("switch-1",  2, SENSOR_TYPE_DIGITAL);
  autogarten.addSensor("soil-temp", ONE_WIRE_PIN, SENSOR_TYPE_ONEWIRE_TEMP);
  autogarten.addSensor("air-temp",  ONE_WIRE_PIN, SENSOR_TYPE_ONEWIRE_TEMP);
  
  // Add actuators
  // TODO...
  
  // Kick off initial sync with control server...
  onSyncWithControlServer();
}


void loop(){
  Alarm.delay(1000);
}


/**
 * The following functions are required since a (non-static) class member function
 * can't be passed into TimeAlarms.
 * TODO: Find a more elegant workaround...
 */
void onSyncWithControlServer() {
  autogarten.syncWithControlServer(onSyncWithControlServer, onReadSensors);
}


void onReadSensors() {
  autogarten.readSensors();
}
