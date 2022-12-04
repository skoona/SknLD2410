/*
 * Example sketch for reporting on readings from the LD2410 using whatever settings are currently configured.
 * 
 * The sketch assumes an ESP32 board with the LD2410 connected as Serial1 to pins 16 & 17, the serial configuration for other boards may vary
 * 
 */

#include <Arduino.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <ld2410.h>

#define RXD2 16 // 8
#define TXD2 17 // 9
// #define SERIAL_STUDIO_TCP 1
#define DNS_PORT 53

#ifdef SERIAL_STUDIO_TCP
AsyncClient client;
static DNSServer DNS;
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const uint16_t port = 7777; //8090;
const char * host = "10.100.1.5";

ld2410 radar;
uint32_t lastReading = 0;
uint32_t pos = 0;
uint32_t pos1 = 0;
uint32_t pos2 = 0;
char buffer1[512];
char serialBuffer[3072];

/*
 * JSON Values for SerialStudio App - see test folder */
int buildLongSerialStudioJSON() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*{\"title\":\"LD2410-Sensor\",\"groups\":[{\"title\":\"Moving Target\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Moving\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Detection\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Energy\",\"alarm\":50,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},{\"title\":\"Stationary Target\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Stationary\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Detection\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Energy\",\"alarm\":50,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},",
    radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"{\"title\":\"Gate %d\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Movement Energy\",\"alarm\":%d,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100},{\"title\":\"Static Energy\",\"alarm\":%d,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},", 
              x, radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "]}*/\n");

  #ifdef SERIAL_STUDIO_TCP
  if (client.connected() > 0) {
    return client.write(serialBuffer, strlen(serialBuffer));
  }
  return 0;
  #else
    return Serial.print(serialBuffer);
  #endif
}

/*
 * JSON Values for SerialStudio App - see test folder */
int buildShortSerialStudioJSON() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*{\"t\":\"LD2410-Sensor\",\"g\":[{\"t\":\"Moving Target\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Moving\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"a\":50,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},{\"t\":\"Stationary Target\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Stationary\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"a\":50,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},",
    radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"{\"t\":\"Gate %d\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Movement Energy\",\"a\":%d,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100},{\"t\":\"Static Energy\",\"a\":%d,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},", 
              x, radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "]}*/\n");

  #ifdef SERIAL_STUDIO_TCP
  if (client.connected() > 0) {
    return client.write(serialBuffer, strlen(serialBuffer));
  }
  return 0;
  #else
    return Serial.print(serialBuffer);
  #endif
}

/*
 * CSV like Values for SerialStudio App - see test folder */
int buildSerialStudioCSV() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*LD2410-Sensor,%d,%d,%d,%d,%d,%d,",radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"%d,%d,", radar.engMovingDistanceGateEnergy(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "*/\n");

  #ifdef SERIAL_STUDIO_TCP
  if (client.connected() > 0) {
    return client.write(serialBuffer, strlen(serialBuffer));
  }
  return 0;
  #else
    return Serial.print(serialBuffer);
  #endif
}

/*
 * CSV like Values for SerialStudio App - see test folder */
int buildWithAlarmSerialStudioCSV() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*LD2410-Sensor,%d,%d,%d,%d,%d,%d,",radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"%d,%d,%d,%d,",  radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "*/\n");

  #ifdef SERIAL_STUDIO_TCP
  if (client.connected() > 0) {
    return client.write(serialBuffer, strlen(serialBuffer));
  }
  return 0;
  #else
    return Serial.print(serialBuffer);
  #endif
}

void setup(void)
{
  delay(1000);

  // start console path
  Serial.begin(115200);
  delay(250);

  // start path to LD2410
  // radar.debug(Serial);
  Serial2.begin (256000, SERIAL_8N1, RXD2, TXD2); //UART for monitoring the radar rx, tx
  Serial.flush();
  delay(250);  

#ifdef SERIAL_STUDIO_TCP
  // Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
        delay(500);
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());

  Serial.println(F("Connecting Client..."));
  
  client.onError([](void* arg, AsyncClient * c, int8_t error) {
    Serial.printf("Error: %s\n\n", c->errorToString(error));
  });
  client.onTimeout([](void* arg, AsyncClient * c, uint32_t time) {
    Serial.printf("Timeout\n\n");
  });
  client.onData([](void *arg, AsyncClient *c, void *data, size_t len) {
	  Serial.printf("\n data received from %s \n", c->remoteIP().toString().c_str());
  });
  client.onConnect([](void* arg, AsyncClient * c) {
    Serial.printf("\n client has been connected to %s on port %d \n", host, port);
  });
  
  if (!DNS.start(DNS_PORT, host, WiFi.softAPIP())) {
		Serial.printf("\n failed to start dns service \n");
  }

  while (!client.connect(host, port)) {
    delay(1000);  
  }
  Serial.println(F("Client Initialized..."));
#endif

  // Start LD2410 Sensor
  if(radar.begin(Serial2))
  {
    // Serial.println(F("Sensor Initialized..."));
    delay(500);
    radar.requestStartEngineeringMode();
  }
  else
  {
    Serial.println(F(" Sensor was not connected"));
  }

  // Serial.println(F("setup() Complete..."));
}

void loop()
{
#ifdef SERIAL_STUDIO_TCP
  DNS.processNextRequest();
#endif

  radar.ld2410_loop();

  if(radar.isConnected() && millis() - lastReading > 1000)  //Report every 1000ms
  {
    lastReading = millis();
    if(radar.presenceDetected())
    {
      #ifdef SERIAL_STUDIO_TCP
      if(!client.connected()) {
        client.connect(host, port);
      }
      #endif
      buildWithAlarmSerialStudioCSV();
    }
  }
}

