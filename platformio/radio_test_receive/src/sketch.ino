#include <SdFat.h>
#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <avr/wdt.h>
//#include <WirelessHEX69.h>
#include "constants.h"
#include "secrets.h"
SdFat sd;
SdFile myFile;

#define NODEID        200    // The Gateway runs on node 200
RFM69 radio;

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

Payload theData;

void setup() {
  Serial.begin(115200);
  Serial.println("starting radio_test_receive");

  // start the SD card
  //if (!sd.begin(4, SPI_HALF_SPEED)) {
    //sd.initErrorHalt();
  //} else {
    //Serial.println(F("SD began"));
    //sd.ls(LS_R);
  //}

  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower(); //uncomment only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  // set promiscuous mode to see all the packets on the network
  // no matter the intended recipient
  //radio.promiscuous(true);
}

uint16_t ackCount = 0;
void loop() {
  if (radio.receiveDone())
  {
    // read out any incoming packets
    if (radio.DATALEN = sizeof(Payload)) {
      Serial.print("Sender:[");Serial.print(radio.SENDERID, DEC);Serial.print("] ");
      theData = *(Payload*)radio.DATA;
      Serial.print(F("message: "));
      Serial.print(theData.message_id);
      Serial.print(F(" impact_num: "));
      Serial.print(theData.impact_num);
      Serial.print(F(" timestamp: "));
      Serial.print(theData.timestamp);
      Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
      Serial.println();
    } else {
      Serial.println(F("Unknown data: "));
      for (byte i = 0; i < radio.DATALEN; i++) {
        Serial.print((char)radio.DATA[i]);
      }
      Serial.println();
    }
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
