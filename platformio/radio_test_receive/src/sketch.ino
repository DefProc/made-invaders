#include <SdFat.h>
#include <RFM69.h>
#include <SPI.h>
SdFat sd;
SdFile myFile;

#define NETWORKID     100  // The same on all nodes that talk to each other
#define NODEID        200    // The Gateway runs on node 200
#define FREQUENCY     RF69_915MHZ
#define IS_RFM69HW
#define ENCRYPTKEY    "changemechangme" //exactly the same 16 characters/bytes on all nodes!

RFM69 radio;

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

typedef struct {
  uint8_t message_id;
  uint32_t impact_num;
  uint32_t timestamp;
} Payload;
Payload theData;

void setup() {
  Serial.begin(9600);
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
    Serial.print("Sender:[");Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    //for (byte i = 0; i < radio.DATALEN; i++)
      //Serial.print((char)radio.DATA[i]);
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      Serial.print(F("message: "));
      Serial.print(theData.message_id);
      Serial.print(F(" impact_num: "));
      Serial.print(theData.impact_num);
      Serial.print(F(" timestamp: "));
      Serial.print(theData.timestamp);
    }
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    if (radio.ACKRequested())
    {
      uint8_t theNodeID = radio.SENDERID;
      radio.sendACK();
      Serial.print(" - ACK sent.");
    }
    Serial.println();
    Blink(LED,3);
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
