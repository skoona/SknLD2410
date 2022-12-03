/*
 * Example sketch for reporting on readings from the LD2410 using whatever settings are currently configured.
 * 
 * The sketch assumes an ESP32 board with the LD2410 connected as Serial1 to pins 16 & 17, the serial configuration for other boards may vary
 * 
 */

/*
{ 
"t":"LD2410 Sensor", 
"g":[ 
      { "t":"Moving Target", 
          "d":[ 
                  { "t":"Moving", "v":"%2", "g":true, "u":"cm" },
                  { "t":"Detection", "v":"%2", "g":true, "u":"cm" },
                  { "t":"Energy", "v":"%2", "g":true },
              ] 
      },
      { "t":"Stationary Target", 
          "d":[ 
                  { "t":"Stationary", "v":"%2", "g":true, "u":"cm" },
                  { "t":"Detection", "v":"%2", "g":true, "u":"cm" },
                  { "t":"Energy", "v":"%2", "g":true,"u":"signal" },
              ] 
      },
      { "t":"Gate 0", 
          "d":[ 
                  { "t":"Movement Energy", "v":"%2", "g":true },
                  { "t":"Static Energy", "v":"%2", "g":true }
              ] 
      },
      
      ...

      { "t":"Gate 7", 
          "d":[ 
                  { "t":"Movement Energy", "v":"%2", "g":true },
                  { "t":"Static Energy", "v":"%2", "g":true }
              ] 
      },
      { "t":"Gate 8", 
          "d":[ 
                  { "t":"Movement Energy", "v":"%2", "g":true },
                  { "t":"Static Energy", "v":"%2", "g":true }
              ] 
      }
    ] 
}
*/

#include <ld2410.h>

#define RXD2 16 // 8
#define TXD2 17 // 9

ld2410 radar;

uint32_t lastReading = 0;
uint32_t pos = 0;
uint32_t pos1 = 0;
uint32_t pos2 = 0;
char buffer1[512];
char serialBuffer[3072];

void setup(void)
{
  delay(1000);
  Serial.begin(115200); //Feedback over Serial Monitor
  delay(100);
  Serial2.begin (256000, SERIAL_8N1, RXD2, TXD2); //UART for monitoring the radar rx, tx
  delay(100);
  Serial.println(F("\nLD2410 radar sensor initialising: "));
  if(radar.begin(Serial2))
  {
    Serial.println(F("OK "));
    delay(0);
    radar.requestStartEngineeringMode();
  }
  else
  {
    Serial.println(F(" not connected"));
  }  
}

/*
 * JSON Values for SerialStudio App - see test folder */
int buildSerialStudioJSON() {
  pos = snprintf(serialBuffer,sizeof(serialBuffer),"/*{\"t\":\"LD2410-Sensor\",\"g\":[{\"t\":\"Moving-Target\",\"d\":[{\"t\":\"Moving\",\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"v\":%d,\"g\":true,\"u\":\"signal\",\"w\":\"bar\",\"min\":0,\"max\":100}]},{\"t\":\"Stationary-Target\",\"d\":[{\"t\":\"Stationary\",\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Detection\",\"v\":%d,\"g\":true,\"u\":\"cm\"},{\"t\":\"Energy\",\"v\":%d,\"g\":true,\"u\":\"signal\",\"w\":\"bar\",\"min\":0,\"max\":100}]},",
    radar.stationaryTargetDistance(),radar.detectionDistance(), radar.stationaryTargetEnergy(),radar.movingTargetDistance(), radar.detectionDistance(), radar.movingTargetEnergy());

  for(int x = 0; x < LD2410_MAX_GATES; ++x) {
    pos1 = snprintf(buffer1,sizeof(buffer1),"{\"t\":\"Gate-%d\",\"d\":[{\"t\":\"Movement-Energy\",\"v\":%d,\"g\":true,\"u\":\"signal\",\"w\":\"bar\",\"min\":0,\"max\":100},{\"t\":\"Static-Energy\",\"v\":%d,\"g\":true,\"u\":\"signal\",\"w\":\"bar\",\"min\":0,\"max\":100}]},", 
              x, radar.engMovingDistanceGateEnergy(x), radar.engStaticDistanceGateEnergy(x));  
    strcat(serialBuffer, buffer1);
    pos += pos1;
  }
  serialBuffer[--pos] = 0;
  strcat(serialBuffer, "]}*/\n");

  return Serial.print(serialBuffer);
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

  return Serial.print(serialBuffer);
}

void loop()
{
  radar.ld2410_loop();

  if(radar.isConnected() && millis() - lastReading > 2000)  //Report every 1000ms
  {
    lastReading = millis();
    if(radar.presenceDetected())
    {
      buildSerialStudioCSV();
    }
  }
}

