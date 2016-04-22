/* Registration Translation

   This sketch translates the relavent game messages back to the expected
   format for Node Red to understand:

    * Game start: 0,<rfid-key>,<game_uid>
    * Take photograph: 1,<rfid-key>,<game_uid>
    * Game end: 2,<rfid-key>,<game_uid>,<score>
    * Target Hit: 3,<rfid-key>,<game_uid>,<target_id>,<timestamp>,<score>
    * Get Ready: 4,<rfid-key>,<game_uid>
*/

#include <RFM69.h>
#include <SPI.h>
#include "constants.h"
#include "secrets.h"

#define NODEID REG_STN
RFM69 radio;

Payload theData;

void setup() {
  // start the serial monitor
  Serial.begin(9600);
  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower();
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(true);
}

void loop() {
  // check any incoming radio messages
  // and pass them to serial
  if (radio.receiveDone()) {
    // check if we're getting hit data
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      switch (theData.message_id) {
        case GAME_START:
          Serial.print("0,");
          printRfid_num();
          Serial.print(",");
          Serial.print(theData.game_uid);
          Serial.println();
          break;
        case TAKE_PHOTO:
          Serial.print("1,");
          printRfid_num();
          Serial.print(",");
          Serial.print(theData.game_uid);
          Serial.println();
          break;
        case GAME_END:
          Serial.print("2,");
          printRfid_num();
          Serial.print(",");
          Serial.print(theData.game_uid);
          Serial.print(",");
          Serial.print(theData.score);
          Serial.println();
          break;
        case HIT:
          Serial.print("3,");
          printRfid_num();
          Serial.print(",");
          Serial.print(theData.game_uid);
          Serial.print(",");
          Serial.print(radio.SENDERID);
          Serial.print(",");
          Serial.print(theData.timestamp);
          Serial.print(",");
          Serial.print(theData.score);
          Serial.print(",");
          Serial.println();
        case GET_READY:
          Serial.print("4,");
          printRfid_num();
          Serial.print(",");
          Serial.print(theData.game_uid);
          Serial.println();
          break;
      }
    }
    if (radio.ACKRequested() && radio.TARGETID == NODEID) {
      uint8_t theNodeID = radio.SENDERID;
      radio.sendACK();
    }
  }
}

void printRfid_num() {
  for (int i=0; i<RFID_DIGITS; i++) {
    Serial.write(theData.rfid_num[i]);
  }
}
