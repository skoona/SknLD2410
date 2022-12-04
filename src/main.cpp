/*
 * Example sketch for reporting on readings from the LD2410 using whatever settings are currently configured.
 * 
 * The sketch assumes an ESP32 board with the LD2410 connected as Serial1 to pins 16 & 17, the serial configuration for other boards may vary
 * 
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <ld2410.h>

#define RXD2 16 // 8
#define TXD2 17 // 9
#define SERIAL_STUDIO 1

#ifdef SERIAL_STUDIO
AsyncUDP udp;
#endif

const char* ssid           = WIFI_SSID;
const char* ssidPassword   = WIFI_PASS;
const uint16_t    sendPort = 8090;
const uint16_t  listenPort = 8090;
const char * remoteHost    = "10.100.1.5";
IPAddress ipSerialStudio(10, 100, 1, 5);
ld2410 radar;
uint32_t lastReading = 0;
uint32_t pos         = 0;
uint32_t pos1        = 0;
uint32_t pos2        = 0;
bool sending_enabled = true;
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

#ifdef SERIAL_STUDIO
  if (udp.connected() > 0) {
    return udp.broadcastTo(serialBuffer,sendPort);
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

#ifdef SERIAL_STUDIO
  if (udp.connected() > 0) {
    return udp.broadcastTo(serialBuffer,sendPort);
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

#ifdef SERIAL_STUDIO
  if (udp.connected() > 0) {
    return udp.broadcastTo(serialBuffer,sendPort);
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

#ifdef SERIAL_STUDIO
  if (udp.connected() > 0) {
    return udp.broadcastTo(serialBuffer,sendPort);
    // return udp.broadcast(serialBuffer,strlen(serialBuffer));
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

#ifdef SERIAL_STUDIO
  // Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, ssidPassword);
  delay(500);
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());

  if(udp.listen(listenPort)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      Serial.print("UDP Packet Type: ");
      Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length()); 
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();
      // String myString = (const char*)packet.data();  
      if(packet.data()[0]=='+') {
        sending_enabled = true;
      } else if(packet.data()[0]=='-') {
        sending_enabled = false;
      }
    });
  }

  // udp.connect(ipSerialStudio,sendPort); // 10.100.1.186

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

  Serial.println(F("setup() Complete..."));
}

void loop()
{
  radar.ld2410_loop();

  if(sending_enabled) {  
    if(radar.isConnected() && millis() - lastReading > 1000)  //Report every 1000ms
    {
      lastReading = millis();
      if(radar.presenceDetected())
      {
        buildWithAlarmSerialStudioCSV();
      }
    }
  }
}

