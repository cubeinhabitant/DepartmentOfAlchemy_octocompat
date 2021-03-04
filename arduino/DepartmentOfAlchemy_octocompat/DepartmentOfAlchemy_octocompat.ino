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

          default:
            DEBUG_PRINT(F("unknown char: "));
            DEBUG_PRINTLN(b);
            Serial.print(F("unk char:"));
            Serial.println(b);
            while (Serial.read() >= 0); // clear any input
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

  Serial.println(F(".OBC"));

  octo_report_config();

  delay(1000);

  Serial.println(F("Ready"));

  DEBUG_PRINTLN(F("Setup complete"));
}

void loop() {
  octo_check_serial();
}
