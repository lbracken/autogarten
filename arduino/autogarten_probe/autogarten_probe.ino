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
 *  + The Arduino WiFi sheild uses the SPI bus and other digital pins.
 *      Do not use pins 7, 10, 11, 12 or 13 in your sketch as they are in use
 *      For more details: http://arduino.cc/en/Main/ArduinoWiFiShield
 *  + Currently pin 9 is reserved for OneWire protocol communication.
 *      Use it for your OneWire devices
 *
 *
 * https://github.com/lbracken/autogarten
 * license: MIT, see LICENSE for more details.
 */
#include <WiFi.h>
#include <OneWire.h>

const int SENSOR_TYPE_TEMP_DIGITAL  = 1;
const int SENSOR_TYPE_TEMP_ONE_WIRE = 2;
const int SENSOR_TYPE_LIGHT_ANALOG  = 3;

// TODO: Currently always setting on fixed pins to use OneWire protocol,
//       however, need to determine a better way to set this on the fly after
//       a user has called addSensor() with a OneWire sensor type.
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


void setup() {
  Serial.begin(9600);
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>> Starting autogarten probe");
  
  // Initialize the probe and wifi configuration
  initProbe("autogarten_probe", "-----", 5000, "changeme");
  initWiFi("-----", "-----", true);
  
  // Add any sensors
  
  // Add any actuators
  
  
}


void loop() {

   delay(10000);
   syncWithControlServer();
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
  Serial.print("... connection successful");
  printWiFiConnectionInfo();
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

/**
 * Wifi debugging function
 * From Arduino 'ConnectWithWPA' WiFi Exmaple
 */
void printWiFiConnectionInfo() {
  Serial.println();
  
  // print the SSID of the network you're attached to:
  Serial.print("    SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);    
  Serial.print("    BSSID: ");
  Serial.print(bssid[5],HEX);
  Serial.print(":");
  Serial.print(bssid[4],HEX);
  Serial.print(":");
  Serial.print(bssid[3],HEX);
  Serial.print(":");
  Serial.print(bssid[2],HEX);
  Serial.print(":");
  Serial.print(bssid[1],HEX);
  Serial.print(":");
  Serial.println(bssid[0],HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("    signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("    Encryption Type:");
  Serial.println(encryption,HEX);
  
  // print your MAC address:
  byte mac[6];  
  WiFi.macAddress(mac);
  Serial.print("    MAC address: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
    
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("    IP Address: ");
  Serial.println(ip);
  Serial.println();  
} 
