/**
 * Firmware for SAMD21 based Department of Alchemy 4 output prop controller
 * that can be programmed by the Octo Banger GUI software.
 * 
 * http://buttonbanger.com/?page_id=164
 * 
 * The original Octo Banger software used the Uno-compatible 1024 byte eeprom
 * for stored the sequence file. The first 1000 bytes were used for the
 * sequence. The upper 24 bytes were used for configuration.
 * 
 * The Department of Alchemy controller will support PWM output but the Octo
 * Banger sofware only supports digital on or off.
 * 
 * One goal of the Octo Banger compatibility software is that the Octo Banger
 * sequence file will be stored on the SD card. This will allow the SD card
 * to be backed up an copied to another SD card so that the SD card can be
 * quickly swapped if needed.
 */

#include <SPI.h>
#include <SD.h>

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x)     SerialUSB.print(x)
  #define DEBUG_PRINTDEC(x)  SerialUSB.print(x, DEC)
  #define DEBUG_PRINTLN(x)   SerialUSB.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
#endif

#define SD_SS 2

File obcSequence;
File obcConfig;
bool obcConfigValid = true;

#define OBC_CFG_FILENAME "OBC.CFG"
#define OBC_SEQ_FILENAME "OBC.SEQ"

#define CFG_LEN 9

volatile uint8_t octo_stamp_buf[5] = {8,2,0,2,6};

void octo_media_ReportConfig() {
  DEBUG_PRINTLN(F("octo_media_ReportConfig"));
  Serial.println(F("Media Type: Catalex MP3 Ambient + Scare"));
  Serial.println(F("Volume: 5"));
}

void octo_report_config() {
  DEBUG_PRINTLN(F("octo_report_config"));
  Serial.print(F("OctoBanger TTL v"));
  for (int i = 0; i < 3; i++) {
    if (i < 2) {
      Serial.print(octo_stamp_buf[i]);
      Serial.print(F("."));
    } else {
      Serial.println(octo_stamp_buf[i]);
    }
  }
  Serial.println(F("Config OK"));
  Serial.println(F("Frame Count: 0"));
  Serial.println(F("Seq Len Secs: 0"));
  Serial.println(F("Reset Delay Secs: 30"));
  Serial.println(F("Pin Map: Default_TTL"));
  delay(300);
  Serial.println(F("Trigger Pin in: 11"));
  Serial.println(F("Trigger Ambient Type: Hi (to gnd trigger)"));
  Serial.println(F("Trigger Pin Out: 10"));
  Serial.println(F("Media Serial Pin: 12"));
  delay(300);
  octo_media_ReportConfig();
  Serial.println(F("Timing Offset ms: -.405"));
  Serial.println(F("TTL PINS: 23456789"));
  Serial.println(F("TTL TYPES: 11111111"));
}

void octo_check_serial() {
  uint8_t b;
  
  if (Serial.available()) {
    b = Serial.read();
    if (b == '@') {
      DEBUG_PRINTLN(F("Command byte received"));
      delay(5);
      if (Serial.available()) {
        b = Serial.read();
        delay(5);
        switch (b) {
          case 'V':
            // return version
            DEBUG_PRINTLN(F("Version"));
            for (int i = 0; i < 3; i++) {
              if (i < 2) {
                Serial.print(octo_stamp_buf[i]);
                Serial.print(F("."));
              } else {
                Serial.println(octo_stamp_buf[i]);
              }
            }
            break;

          case 'H':
            // go hot
            // TODO: implement
            DEBUG_PRINTLN(F("Not implemented: H - go hot"));
            Serial.println(F("Not implemented: H - go hot"));
            flushReceive();
            break;

          case 'C':
            // go cold
            // TODO: implement
            DEBUG_PRINTLN(F("Not implemented: C - go cold"));
            Serial.println(F("Not implemented: C - go cold"));
            flushReceive();
            break;

          case 'D':
            // download eeprom
            DEBUG_PRINTLN(F("Not implemented: D - download eeprom"));
            Serial.println(F("Not implemented: D - download eeprom"));
            flushReceive();
            break;

          case 'F':
            // download config eeprom
            DEBUG_PRINTLN(F("Not implemented: F - download config eeprom"));
            Serial.println(F("Not implemented: F - download config eeprom"));
            flushReceive();
            break;

          case 'S':
            // receive sequence data
            DEBUG_PRINTLN(F("Not implemented: S - upload sequence data"));
            Serial.println(F("Not implemented: S - upload sequence data"));
            flushReceive();
            break;

          case 'U':
            // upload config
            DEBUG_PRINTLN(F("Upload config"));
            octo_receiveConfig();
            break;

          case 'P':
            // ping back config
            DEBUG_PRINTLN(F("Not implemented: P - ping back config"));
            Serial.println(F("Not implemented: P - ping back config"));
            flushReceive();
            break;

          case 'T':
            // trigger
            DEBUG_PRINTLN(F("Not implemented: T - trigger"));
            Serial.println(F("Not implemented: T - trigger"));
            flushReceive();
            break;

          case 'M':
            // manual output
            DEBUG_PRINTLN(F("Not implemented: M - manual output"));
            Serial.println(F("Not implemented: M - manual output"));
            flushReceive();
            break;

          case 'O':
            // stamp OK
            DEBUG_PRINTLN(F("Not implemented: O - stamp OK"));
            Serial.println(F("Not implemented: O - stamp OK"));
            flushReceive();
            break;

          default:
            DEBUG_PRINT(F("unknown char: "));
            DEBUG_PRINTLN(b);
            Serial.print(F("unk char:"));
            Serial.println(b);
            flushReceive();
            break;
        }
      }
    } else {
      DEBUG_PRINT(F("Not command byte received: "));
      DEBUG_PRINTLN(b);
    }
  }
}

void setup() {
  Serial.begin(115200); // peripheral prop controller (OctoBanger)

#ifdef DEBUG
  SerialUSB.begin(115200); // serial debug
  while (!SerialUSB); // wait for serial port to connect. Needed for native USB port only
#endif

  DEBUG_PRINTLN(F("Department of Alchemy - octocompat"));

  // let computer know we are listening
  Serial.println(F(".OBC"));

  DEBUG_PRINTLN(F("Initializing SD card..."));

  if (!SD.begin(SD_SS)) {
    DEBUG_PRINTLN(F("initialization failed!"));
    while (1);
  }
  DEBUG_PRINTLN(F("initialization done."));

  octo_report_config();

  delay(1000);

  Serial.println(F("Ready"));

  DEBUG_PRINTLN(F("Setup complete"));
}

void loop() {
  octo_check_serial();
}

/**
 *  Clear the receive buffer of any characters
 */
void flushReceive() {
  while (Serial.read() >= 0); // clear any input
}

uint16_t readInt16() {
  uint8_t b[2];

  for (int i = 0; i < 2; i++) {
    while (Serial.available() == 0) {}
    b[i] = Serial.read();
  }
  
  return (uint16_t)(((uint16_t)b[1] << 8) | b[0]);
}

void octo_receiveConfig() {
  uint8_t b;
  uint16_t len;
  while (Serial.available() == 0) {}

  len = readInt16();

  if (len != CFG_LEN) {
    Serial.print(F("Unexpected config length: "));
    Serial.println(len);
    flushReceive();
    return;
  }

  if (SD.exists(OBC_CFG_FILENAME)) {
    SD.remove(OBC_CFG_FILENAME);
  }
  obcConfig = SD.open(OBC_CFG_FILENAME, FILE_WRITE);
  if (!obcConfig) {
    Serial.print(F("Unabled to open config file for writing: "));
    Serial.println(OBC_CFG_FILENAME);
    return;
  }

  for (uint16_t i = 0; i < len; i++) {

    uint16_t count = 0;
    while (Serial.available() == 0) {
      count++;
      if (count > 0xFFFE) {
        Serial.print(F("Timeout waiting for config information: "));
        Serial.println(count);
        Serial.println("Please try again");
        flushReceive();
        return;
      }
    }
    b = Serial.read();
    obcConfig.write(b);
  }

  Serial.print(F("Received "));
  Serial.print(len);
  Serial.println(F(" config bytes"));
  Serial.println(F("Please reconnect"));

  // write 8 reserved bytes
  for (uint8_t i = 0; i < 8; i++) {
    obcConfig.write((uint8_t)0x00);
  }

  // write stamp
  for (uint8_t i = 0; i < 5; i++) {
    obcConfig.write(octo_stamp_buf[i]);
  }

  obcConfig.close();

  if (!obcConfigValid) {
    // clear any sequence file if the config data is not valid
    if (SD.exists(OBC_SEQ_FILENAME)) {
      SD.remove(OBC_SEQ_FILENAME);
    }
  }

  obcConfigValid = true;

  // OBF would now execute read_config to intialize variables based on the
  // config data.
  // TODO: initialize firmware based on translation of config values
}