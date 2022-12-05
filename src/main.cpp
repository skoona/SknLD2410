/*
 * Example sketch for reporting on readings from the LD2410 using whatever settings are currently configured.
 * 
 * The sketch assumes an ESP32 board with the LD2410 connected as Serial2 on pins 16 & 17, the serial configuration for other boards may vary
 * 
 * Program broadcasts reporting data on port 8090,, and listens on port 8091 for commands or configuration requests, response to callers ip/port as discovered.
 * 
 */

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <ld2410.h>

#define RXD2 16 // 8
#define TXD2 17 // 9
#define SNAME "LD2410 Sensor 01"
#define SERIAL_STUDIO_HOSTIP "10.100.1.5"
#define SERIAL_STUDIO 1

#ifdef SERIAL_STUDIO
AsyncUDP udp;
#endif

const char* ssid           = WIFI_SSID;
const char* ssidPassword   = WIFI_PASS;
const uint16_t    sendPort = 8090;
const uint16_t  listenPort = 8091;
IPAddress ipSerialStudio(10,100,1,5);

uint16_t  remotePort = 8090;     // default value, will be overridden on reciept of udp request
IPAddress ipRemote(10,100,1,5);  // default value, will be overridden on reciept of udp request

ld2410 radar;
volatile bool udpFlag = false; // send for callback
uint32_t lastReading = 0;
uint32_t pos         = 0;
uint32_t pos1        = 0;
uint32_t pos2        = 0;
bool sending_enabled = true;
char buffer1[512];
char serialBuffer[3072];
String command;

/*
 * Command Processor */
String commandProcessor(String &cmdStr) {
  String sBuf = "\n";
  cmdStr.trim();

  if(cmdStr.equals("help")) 
  {
    sBuf += "\nSupported commands:\n\tthelp: this text.\n\tstreamstart: start sending udp data to SerialStudio.\n\tstreamstop: stop sending to SerialStream.\n\tread: read current values from the sensor\n\treadconfig: read the configuration from the sensor\n\tsetmaxvalues <motion gate> <stationary gate> <inactivitytimer>\n\tsetsensitivity <gate> <motionsensitivity> <stationarysensitivity>\n\tenableengineeringmode: enable engineering mode\n\tdisableengineeringmode: disable engineering mode\n\trestart: restart the sensor\n\treadversion: read firmware version\n\tfactoryreset: factory reset the sensor\n";
    sBuf += "\n\n choose> ";
  }
  else if(cmdStr.equals("streamstart")) 
  {
      sending_enabled = true;      
      sBuf += "\nSerialStudio UDP Stream Enabled. \n"; 
  } 
  else if(cmdStr.equals("streamstop")) 
  {
      sending_enabled = false;
      sBuf += "\nSerialStudio UDP Stream Disabled.\n";
  }
  else if(cmdStr.equals("read"))
  {
    sBuf += "\nReading from sensor: ";
    if(radar.isConnected())
    {
      sBuf += "OK\n";
      if(radar.presenceDetected())
      {
        if(radar.stationaryTargetDetected())
        {
          sBuf += "Stationary target: ";
          sBuf += radar.stationaryTargetDistance();
          sBuf += "cm energy: ";
          sBuf += radar.stationaryTargetEnergy();
          sBuf += "\n";
        }
        if(radar.movingTargetDetected())
        {
          sBuf += "Moving target: ";
          sBuf += radar.movingTargetDistance();
          sBuf += "cm energy: ";
          sBuf += radar.movingTargetEnergy();
          sBuf += "\n";
        }
      }
      else
      {
        sBuf += "\nnothing detected\n";
      }
    }
    else
    {
      sBuf += "failed to read\n";
    }
  }
  else if(cmdStr.equals("readconfig"))
  {
    sBuf += "\nReading configuration from sensor: ";
    if(radar.requestCurrentConfiguration())
    {
      sBuf += "OK\n";
      sBuf += "Maximum gate ID: ";
      sBuf += radar.cfgMaxGate();
      sBuf += "\n";
      sBuf += "Maximum gate for moving targets: ";
      sBuf += radar.cfgMaxMovingGate();
      sBuf += "\n";
      sBuf += "Maximum gate for stationary targets: ";
      sBuf += radar.cfgMaxStationaryGate();
      sBuf += "\n";
      sBuf += "Idle time for targets: "; 
      sBuf += radar.cfgSensorIdleTimeInSeconds() ;
      sBuf += "\n";
      sBuf += "Gate sensitivity\n";
      
      for(uint8_t gate = 0; gate < LD2410_MAX_GATES; gate++)
      {
        sBuf += "Gate ";
        sBuf += gate;
        sBuf += " moving targets: ";
        sBuf += radar.cfgMovingGateSensitivity(gate);
        sBuf += " stationary targets: ";
        sBuf += radar.cfgStationaryGateSensitivity(gate);
        sBuf += "\n";
      }
    }
    else
    {
      sBuf += "Failed\n";
    }
  }
  else if(cmdStr.startsWith("setmaxvalues"))
  {
    uint8_t firstSpace = cmdStr.indexOf(' ');
    uint8_t secondSpace = cmdStr.indexOf(' ',firstSpace + 1);
    uint8_t thirdSpace = cmdStr.indexOf(' ',secondSpace + 1);
    uint8_t newMovingMaxDistance = (cmdStr.substring(firstSpace,secondSpace)).toInt();
    uint8_t newStationaryMaxDistance = (cmdStr.substring(secondSpace,thirdSpace)).toInt();
    uint16_t inactivityTimer = (cmdStr.substring(thirdSpace,cmdStr.length())).toInt();
    if(newMovingMaxDistance > 0 && newStationaryMaxDistance > 0 && newMovingMaxDistance <= 8 && newStationaryMaxDistance <= 8)
    {
      sBuf += "\nSetting max values to gate ";
      sBuf += newMovingMaxDistance;
      sBuf += " moving targets, gate ";
      sBuf += newStationaryMaxDistance;
      sBuf += " stationary targets, ";
      sBuf += inactivityTimer;
      sBuf += "s inactivity timer: ";
      if(radar.setMaxValues(newMovingMaxDistance, newStationaryMaxDistance, inactivityTimer))
      {
        sBuf += "OK, now restart to apply settings\n";
      }
      else
      {
        sBuf += "failed\n";
      }
    }
    else
    {
      sBuf += "Can't set distances to ";
      sBuf += newMovingMaxDistance;
      sBuf += " moving ";
      sBuf += newStationaryMaxDistance;
      sBuf += " stationary, try again\n";
    }
  }
  else if(cmdStr.startsWith("setsensitivity"))
  {
    uint8_t firstSpace = cmdStr.indexOf(' ');
    uint8_t secondSpace = cmdStr.indexOf(' ',firstSpace + 1);
    uint8_t thirdSpace = cmdStr.indexOf(' ',secondSpace + 1);
    uint8_t gate = (cmdStr.substring(firstSpace,secondSpace)).toInt();
    uint8_t motionSensitivity = (cmdStr.substring(secondSpace,thirdSpace)).toInt();
    uint8_t stationarySensitivity = (cmdStr.substring(thirdSpace,cmdStr.length())).toInt();
    if(motionSensitivity >= 0 && stationarySensitivity >= 0 && motionSensitivity <= 100 && stationarySensitivity <= 100)
    {
      sBuf += "\nSetting gate ";
      sBuf += gate;
      sBuf += " motion sensitivity to ";
      sBuf += motionSensitivity;
      sBuf += " & stationary sensitivity to ";
      sBuf += stationarySensitivity;
      sBuf += ": \n";
      if(radar.setGateSensitivityThreshold(gate, motionSensitivity, stationarySensitivity))
      {
        sBuf += "OK, now restart to apply settings\n";
      }
      else
      {
        sBuf += "failed\n";
      }
    }
    else
    {
      sBuf += "Can't set gate ";
      sBuf += gate;
      sBuf += " motion sensitivity to ";
      sBuf += motionSensitivity;
      sBuf += " & stationary sensitivity to ";
      sBuf += stationarySensitivity;
      sBuf += ", try again\n";
    }
  }
  else if(cmdStr.equals("enableengineeringmode"))
  {
    sBuf += "\nEnabling engineering mode: ";
    if(radar.requestStartEngineeringMode())
    {
      sBuf += "OK\n";
    }
    else
    {
      sBuf += "failed\n";
    }
  }
  else if(cmdStr.equals("disableengineeringmode"))
  {
    sBuf += "\nDisabling engineering mode: ";
    if(radar.requestEndEngineeringMode())
    {
      sBuf += "OK\n";
    }
    else
    {
      sBuf += "failed\n";
    }
  }
  else if(cmdStr.equals("restart"))
  {
    sBuf += "\nRestarting sensor: ";
    if(radar.requestRestart())
    {
      sBuf += "OK\n";
    }
    else
    {
      sBuf += "failed\n";
    }
  }
  else if(cmdStr.equals("readversion"))
  {
    sBuf += "\nRequesting firmware version: ";
    if(radar.requestFirmwareVersion())
    {
      sBuf += radar.cmdFirmwareVersion();
    }
    else
    {
      sBuf += "Failed\n";
    }
  }
  else if(cmdStr.equals("factoryreset"))
  {
    sBuf += "\nFactory resetting sensor: ";
    if(radar.requestFactoryReset())
    {
      sBuf += "OK, now restart sensor to take effect\n";
    }
    else
    {
      sBuf += "failed\n";
    }
  }
  else
  {
    sBuf += "\nUnknown command: ";
    sBuf += cmdStr;
    sBuf += "\n";
  }

  cmdStr.clear();
  sBuf += "\n\n choose:> ";

  return sBuf;
}


void commandHandler() {
  if(Serial.available())
  {
    char typedCharacter = Serial.read();
    if(typedCharacter == '\n') {
      if(typedCharacter !='\r') {
        Serial.print( commandProcessor(command) );        
      }
    } else {
      Serial.print(typedCharacter);
      command += typedCharacter;
    }
  }
}


/*
 * Send data via UDP */
void sendToRequestor(String str, bool requestor = false) {
  if(requestor) {
    udp.connect(ipRemote,remotePort);
  }  else {
    udp.connect(ipSerialStudio,sendPort);
  }
  udp.print(str);
  return udp.close();
}

/*
 * JSON Values for SerialStudio App - see test folder */
int buildLongSerialStudioJSON() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*{\"title\":\"%s\",\"groups\":[{\"title\":\"Moving Target\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Moving\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Detection\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Energy\",\"alarm\":50,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},{\"title\":\"Stationary Target\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Stationary\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Detection\",\"alarm\":0,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"cm\"},{\"title\":\"Energy\",\"alarm\":50,\"led\":false,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},",
    SNAME, radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"{\"title\":\"Gate %d\",\"widget\":\"multiplot\",\"datasets\":[{\"title\":\"Movement Energy\",\"alarm\":%d,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100},{\"title\":\"Static Energy\",\"alarm\":%d,\"value\":%d,\"graph\":true,\"units\":\"signal\",\"min\":0,\"max\":100}]},", 
              x, radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "]}*/\n");

#ifdef SERIAL_STUDIO
  sendToRequestor(String(serialBuffer));
  return 0;
#else
  return Serial.print(serialBuffer);
#endif
}

/*
 * JSON Values for SerialStudio App - see test folder */
int buildShortSerialStudioJSON() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*{\"t\":\"%s\",\"g\":[{\"t\":\"Moving Target\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Moving\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"a\":50,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},{\"t\":\"Stationary Target\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Stationary\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"a\":0,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"a\":50,\"l\":false,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},",
    SNAME, radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"{\"t\":\"Gate %d\",\"w\":\"multiplot\",\"d\":[{\"t\":\"Movement Energy\",\"a\":%d,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100},{\"t\":\"Static Energy\",\"a\":%d,\"v\":%d,\"g\":true,\"u\":\"signal\",\"min\":0,\"max\":100}]},", 
              x, radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "]}*/\n");

#ifdef SERIAL_STUDIO
  sendToRequestor(String(serialBuffer));
  return 0;
#else
  return Serial.print(serialBuffer);
#endif
}

/*
 * CSV like Values for SerialStudio App - see test folder */
int buildSerialStudioCSV() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*%s,%d,%d,%d,%d,%d,%d,",SNAME,radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"%d,%d,", radar.engMovingDistanceGateEnergy(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "*/\n");

#ifdef SERIAL_STUDIO
  sendToRequestor(String(serialBuffer));
  return 0;
#else
  return Serial.print(serialBuffer);
#endif
}

/*
 * CSV like Values for SerialStudio App - see test folder */
int buildWithAlarmSerialStudioCSV() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*%s,%d,%d,%d,%d,%d,%d,",SNAME,radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"%d,%d,%d,%d,",  radar.cfgMovingGateSensitivity(x), radar.engMovingDistanceGateEnergy(x), radar.cfgStationaryGateSensitivity(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "*/\n");

#ifdef SERIAL_STUDIO
  sendToRequestor(String(serialBuffer));
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  // 10.100.1.186

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

    // save path for response when using udp
    ipRemote = packet.remoteIP();
    remotePort = packet.remotePort();

    // Parse Commands -- executing inside a callback can be problematic
    // Using udpFlag to have loop handle it
    command = (const char*)packet.data(); 
    udpFlag=true;
  });

  if(udp.listen(listenPort)) {
    Serial.print(F("Client Listening on port: "));
    Serial.println(listenPort);
  }

  Serial.println(F("Client Initialized..."));
#endif

  // Start LD2410 Sensor
  if(radar.begin(Serial2))
  {
    Serial.println(F("Sensor Initialized..."));
    delay(500);
    radar.requestStartEngineeringMode();
  }
  else
  {
    Serial.println(F(" Sensor was not connected"));
  }

  Serial.println(F("setup() Complete..."));
  Serial.println(F("\nSupported commands:\n\thelp: this text.\n\tstreamstart: start sending udp data to SerialStudio.\n\tstreamstop: stop sending to SerialStream.\n\tread: read current values from the sensor\n\treadconfig: read the configuration from the sensor\n\tsetmaxvalues <motion gate> <stationary gate> <inactivitytimer>\n\tsetsensitivity <gate> <motionsensitivity> <stationarysensitivity>\n\tenableengineeringmode: enable engineering mode\n\tdisableengineeringmode: disable engineering mode\n\trestart: restart the sensor\n\treadversion: read firmware version\n\tfactoryreset: factory reset the sensor\n"));
  Serial.print("\n\n choose> ");
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
  if(udpFlag && (command.length() > 1)) { // handle cb request
    sendToRequestor(commandProcessor(command), true);
    udpFlag=false;
    command.clear();
  }
  commandHandler();
}
//              %1,2,3, 4, 5,6,7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43
/*LD2410 Sensor 01,0,0,62,43,0,0,50,15, 0, 0,50,15, 0, 0,40, 5,40,62,30, 9,40,45,20, 3,30,25,15, 6,30,18,15, 1,20,10,15, 2,20, 8,15, 7,20, 6*/