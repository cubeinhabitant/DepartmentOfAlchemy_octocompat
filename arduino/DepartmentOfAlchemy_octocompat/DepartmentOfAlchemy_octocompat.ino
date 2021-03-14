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

#define OBC_CFG_FILENAME "OBC.CFG"
#define OBC_SEQ_FILENAME "OBC.SEQ"

#define CFG_LEN 9

// obc config offset
const uint16_t OBC_PIN_MAP_SLOT = 0;
const uint16_t OBC_TTL_TYPES_SLOT = 1;
const uint16_t OBC_MEDIA_TYPE_SLOT = 2;
const uint16_t OBC_MEDIA_DELAY_SLOT = 3;
const uint16_t OBC_RESET_DELAY_SLOT = 4;
const uint16_t OBC_BOOT_DELAY_SLOT = 5;
const uint16_t OBC_VOLUME_SLOT = 6;
const uint16_t OBC_TRIGGER_TYPE_SLOT = 7;
const uint16_t OBC_TIMING_OFFSET_TYPE_SLOT = 8;

#define OBC_MEDIA_TYPE_NONE 0x0
#define OBC_MEDIA_TYPE_CATALEX_SCARE 0x1
#define OBC_MEDIA_TYPE_CATALEX_AMB_SCARE 0x2
#define OBC_MEDIA_TYPE_SPRITE 0x3

const uint8_t OBC_TRIGGER_PIN_COUNT = 3;
uint8_t OBC_TRIGGER_PINS[OBC_TRIGGER_TYPE_SLOT] = {11, 10, 12};
const uint8_t OBC_TIMING_OFFSET_TYPE_COUNT = 2;
const float OBC_TIMING_OFFSETS[OBC_TIMING_OFFSET_TYPE_COUNT] = {-.405, 0};
volatile float OBC_MILLIS_PER_FRAME = 50.0; // 49.595; // default is 20 frames per sec (50ms each)
volatile uint8_t octo_stamp_buf[5] = {8,2,0,2,6};

File obcSequence;
volatile bool obcConfigValid = true;
volatile uint8_t obcConfig[CFG_LEN];
// OBC calls this "hi_sample". Uses it to tell how much of the eeprom is
// in use. We can just call it sequence file size (in bytes) / 2.
volatile uint16_t obcFrameCount = 0x0000;
volatile uint8_t obcTriggerType = 0x1;
volatile uint8_t obcPinMap = 0x0;
volatile uint8_t obcResetDelaySecs = 30;
volatile uint8_t obcBootDelaySecs = 0x0;
volatile uint8_t obcVolume = 22;
volatile uint8_t obcMediaType = 0x0;
volatile uint8_t obcMediaDelay = 0x0;
volatile uint8_t obcTimingOffsetType = 0x0;

void octo_media_ReportConfig() {
  DEBUG_PRINTLN(F("octo_media_ReportConfig"));
  Serial.print(F("Media Type: "));
  switch (obcMediaType) {
  case OBC_MEDIA_TYPE_NONE: Serial.println(F("None")); break;
  case OBC_MEDIA_TYPE_CATALEX_SCARE: Serial.println(F("Catalex MP3 Scare Only")); break;
  case OBC_MEDIA_TYPE_CATALEX_AMB_SCARE: Serial.println(F("Catalex MP3 Ambient + Scare")); break;
  case OBC_MEDIA_TYPE_SPRITE: Serial.println(F("Sprite Video")); break;
  default: Serial.println(F("Unknown!")); return;
  }
  Serial.print(F("Volume: "));
  Serial.println(obcVolume);
  if (obcMediaDelay != 0) {
    Serial.print(F("Media Delay: "));
    Serial.println(obcMediaDelay);
  }
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
  if (obcConfigValid) {
    Serial.println(F("Config OK"));
  } else {
    Serial.println(F("Config NOT FOUND, using defaults"));
  }
  Serial.print(F("Frame Count: "));
  Serial.println(obcFrameCount);

  Serial.println(F("Seq Len Secs: 0"));
  Serial.print(F("Reset Delay Secs: "));
  Serial.println(obcResetDelaySecs);
  if (obcBootDelaySecs != 0) {
    Serial.print(F("Boot Delay Secs: "));
    Serial.println(obcBootDelaySecs);
  }
  Serial.print(F("Pin Map: "));
  switch(obcPinMap) {
  case 0x0:
    Serial.println(F("Default_TTL"));
    break;
  case 0x1:
    Serial.println(F("Shield"));
    break;
  case 0x2:
    Serial.println(F("Custom"));
    break;
  case 0x3:
    Serial.println(F("Unknown"));
    break;
  }
  delay(300);
  Serial.print(F("Trigger Pin in: "));
  Serial.println(OBC_TRIGGER_PINS[0]);
  Serial.print(F("Trigger Ambient Type: "));
  if (obcTriggerType == 0) {
    Serial.println(F("Low (PIR or + trigger)"));
  } else {
    Serial.println(F("Hi (to gnd trigger)"));
  }
  Serial.print(F("Trigger Pin Out: "));
  Serial.println(OBC_TRIGGER_PINS[1]);
  Serial.print(F("Media Serial Pin: "));
  Serial.println(OBC_TRIGGER_PINS[2]);
  delay(300);
  octo_media_ReportConfig();
  Serial.print(F("Timing Offset ms: "));
  Serial.println(OBC_TIMING_OFFSETS[obcTimingOffsetType], 3);
  Serial.println(F("TTL PINS: 23456789")); // TODO: implement
  Serial.println(F("TTL TYPES: 11111111")); // TODO: implement
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
            DEBUG_PRINTLN(F("Receive sequence data"));
            octo_rx_seq_data();
            break;

          case 'U':
            // upload config
            DEBUG_PRINTLN(F("Upload config"));
            octo_rx_controller_config();
            break;

          case 'P':
            // ping back config
            DEBUG_PRINTLN(F("Ping back config"));
            octo_report_config();
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
            // config (stamp) OK
            DEBUG_PRINTLN(F("Config (stamp) OK"));
            if (obcConfigValid) {
              Serial.println(F("OK"));
            } else {
              Serial.println(F("NO"));
            }
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

  DEBUG_PRINTLN(F("Initializing SD card..."));

  if (!SD.begin(SD_SS)) {
    DEBUG_PRINTLN(F("initialization failed!"));
    while (1);
  }
  DEBUG_PRINTLN(F("initialization done."));

  octo_read_config();

  // let computer know we are listening
  Serial.println(F(".OBC"));

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

void octo_rx_seq_data() {
  uint8_t b;
  uint16_t len;
  File obcSequenceFile;
  float duration = 0;

  while (Serial.available() == 0) {}

  len = readInt16();

  if (SD.exists(OBC_SEQ_FILENAME)) {
    SD.remove(OBC_SEQ_FILENAME);
  }
  obcSequenceFile = SD.open(OBC_SEQ_FILENAME, FILE_WRITE);
  if (!obcSequenceFile) {
    Serial.print(F("Unabled to open sequence file for writing: "));
    Serial.println(OBC_SEQ_FILENAME);
    return;
  }

  // max of arduino uno/nano based octobanger is 1000 bytes
  // so they all fit in the arduino eeprom
  for (uint16_t i = 0; i < len && i < 1000; i++) {
    uint16_t count = 0;
    while (Serial.available() == 0) {
      count++;
      if (count > 0xFFFE) {
        Serial.print(F("Stuck on: "));
        Serial.println(i);
        Serial.println("Upload terminated, please retry");
        return;
      }
    }
    b = Serial.read();
    obcSequenceFile.write(b);

    // calcuting the duration while receiving
    if ((i % 2) == 1) {
      duration += (float)(b) * OBC_MILLIS_PER_FRAME;
    }
  }

  obcSequenceFile.close();

  obcFrameCount = len / 2;
  Serial.print(F("received "));
  Serial.print(obcFrameCount);
  Serial.println(F(" frames"));

  delay(300);

  Serial.print(F("Seq Len Secs: "));
  Serial.println(duration / 1000.0);

  // TODO: initialize firmware based on translation of config values

  Serial.println(F("Saved, Ready"));
}

void octo_rx_controller_config() {
  uint8_t b;
  uint16_t len;
  File obcConfigFile;

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
  obcConfigFile = SD.open(OBC_CFG_FILENAME, FILE_WRITE);
  if (!obcConfigFile) {
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
    obcConfigFile.write(b);
  }

  Serial.print(F("Received "));
  Serial.print(len);
  Serial.println(F(" config bytes"));
  Serial.println(F("Please reconnect"));

  // write 8 reserved bytes
  for (uint8_t i = 0; i < 8; i++) {
    obcConfigFile.write((uint8_t)0x00);
  }

  // write stamp
  for (uint8_t i = 0; i < 5; i++) {
    obcConfigFile.write(octo_stamp_buf[i]);
  }

  obcConfigFile.close();

  if (!obcConfigValid) {
    // clear any sequence file if the config data is not valid
    if (SD.exists(OBC_SEQ_FILENAME)) {
      SD.remove(OBC_SEQ_FILENAME);
    }
  }

  octo_read_config();

  // TODO: initialize firmware based on translation of config values
}

void octo_read_config() {
  File obcFile;
  uint8_t b;

  DEBUG_PRINTLN(F("octo_read_config"));

  obcConfigValid = false;

  if (!SD.exists(OBC_CFG_FILENAME)) {
    DEBUG_PRINTLN(F("config file not found on SD card"));
    return;
  }

  obcFile = SD.open(OBC_CFG_FILENAME, FILE_READ);
  if (!obcFile) {
    Serial.print(F("Unabled to open config file for writing: "));
    Serial.println(OBC_CFG_FILENAME);
    return;
  }

  if (obcFile.size() != (CFG_LEN + 8 + 5)) {
    Serial.print(F("Unexpected config file size: "));
    Serial.println(obcFile.size());
    return;
  }

  // read config
  for (uint8_t i = 0; i < CFG_LEN; i++) {
    obcConfig[i] = obcFile.read();
  }

  // read 8 reserved bytes
  for (uint8_t i = 0; i < 8; i++) {
    obcFile.read();
  }

  // read stamp
  for (uint8_t i = 0; i < 5; i++) {
    b = obcFile.read();
    if (b != octo_stamp_buf[i]) {
      Serial.print(F("Unexpected stamp["));
      Serial.print(i);
      Serial.print(F("]: "));
      Serial.print(b);
      break;
    }
  }
  // got this far, the config is valid
  obcConfigValid = true;

  obcFile.close();

  // initialize configuration based values
  obcPinMap = obcConfig[OBC_PIN_MAP_SLOT];
  // TODO: OBC_TTL_TYPES_SLOT;
  obcMediaType = obcConfig[OBC_MEDIA_TYPE_SLOT];
  obcMediaDelay = obcConfig[OBC_MEDIA_DELAY_SLOT];
  obcResetDelaySecs = obcConfig[OBC_RESET_DELAY_SLOT];
  obcBootDelaySecs = obcConfig[OBC_BOOT_DELAY_SLOT];
  obcVolume = obcConfig[OBC_VOLUME_SLOT];
  obcTriggerType = obcConfig[OBC_TRIGGER_TYPE_SLOT];
  obcTimingOffsetType = obcConfig[OBC_TIMING_OFFSET_TYPE_SLOT];

  if (obcTimingOffsetType > OBC_TIMING_OFFSET_TYPE_COUNT - 1) {
    obcTimingOffsetType = 0x0;
  }

  // check to see if there is a sequence file. if so, find out how big it is
  if (!SD.exists(OBC_SEQ_FILENAME)) {
    obcFrameCount = 0x0000;
    return;
  }

  obcFile = SD.open(OBC_SEQ_FILENAME, FILE_READ);
  obcFrameCount = obcFile.size() / 2;

  obcFile.close();
}