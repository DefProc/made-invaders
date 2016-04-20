#include <SD.h>
#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <avr/wdt.h>
#include <WirelessHEX69.h>
#include <Wire.h>
#include <SL018.h>
#include <RTClib.h>
#include <Adafruit_VS1053.h>
#include "constants.h"
#include "secrets.h"

SL018 rfid;
RFM69 radio;
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

// set the appropiate node ID
#define NODEID MAIN_CTRL    // The Gateway runs on node 200

// VS1053 connections
#define VS1053_CS     0      // VS1053 chip select pin (output)
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ A0       // VS1053 Data request, ideally an Interrupt pin
// No free interrupt pin on the Moteino Mega so trying a non-interupt pin
#define VS1053_RESET  A1      // VS1053 reset pin (output)
#define VS1053_DCS    A2      // VS1053 Data/command select pin (output)
#define CARDCS        A3     // Card chip select pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, DREQ, CARDCS);

// button and led connections
#define START_BUTTON  10
#define CANCEL_BUTTON 11
#define START_LED     15

// Lets get the global variables some space
Payload theData; // somewhere to store incoming data
Payload myPacket; // and one for oddutgoing
game_states_t game_state = IDLE;
char player_rfid[RFID_DIGITS+1];
long
  current_score = 0;
unsigned long
  start_time = 0,
  game_uid = 0;
volatile bool
  update_scoreboard = false,
  take_photo = false;

// set the target scores:
long scoremap[] = { 10, 20, 50, 100, 1234, 5120, -100, 999, 1024, 512, 256, 128, 200, 865, 6547, 450 };
unsigned long hit_record[16];

void setup() {
  Serial.begin(BAUD);
  Serial.println("starting made-invaders main_controller…");

  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower();
#endif
  radio.encrypt(ENCRYPTKEY);

  pinMode(START_LED, OUTPUT);
  pinMode(START_BUTTON, INPUT);
  pinMode(CANCEL_BUTTON, INPUT);

  Blink(START_LED, 1000);
  Serial.println(F("end of setup"));
}

uint16_t ackCount = 0;
void loop() {
  // start the state machine
  if (game_state == IDLE) {
    Serial.println(F("IDLE state"));
    // we're waiting for an RFID scan
    // TODO: reactivate this once we get a level converter installed!
    //rfid.seekTag();
    bool nothing_doing = false; // have we sent a "nothing doing" message
    // hold for rfid read
    Serial.println(F("waiting for rfid tag"));
    while (!rfid.available() && digitalRead(CANCEL_BUTTON) == LOW) {
      // TODO: remove the cancel button check that bypasses the rfid scan
      if (nothing_doing == false) {
        if (millis() - start_time >= IDLE_TIMER) {
          Serial.println(F("nothing doing"));
          // broadcast NOTHING_DOING to go to idle mode
          myPacket.game_uid = 0;
          myPacket.impact_num = 0;
          myPacket.score = 0;
          for (int i=0; i<sizeof(myPacket.rfid_num); i++) {
            myPacket.rfid_num[i] = '0';
          }
          myPacket.timestamp = 0;
          myPacket.message_id = NOTHING_DOING;
          radio.send(0xFF, (const void*)&myPacket, sizeof(myPacket), false);
          nothing_doing = true;
        }
      }
    }
    strcpy(player_rfid, rfid.getTagString());
    Serial.print(F("rfid: "));
    Serial.println(player_rfid);
    // copy all the game variables into the right places for sending
    strcpy(player_rfid, myPacket.rfid_num);
    myPacket.game_uid = game_uid;
    myPacket.impact_num = 0;
    myPacket.score = 0;
    myPacket.timestamp = 30000;
    // let everybody know what's happening
    broadcastMessage(GET_READY);
    // lets at least try to make sure the registration station gets this
    //radio.sendWithRetry(REG_STN, (const void*)&myPacket, sizeof(myPacket), 5);
    game_state = RFID_SCANNED;
  } else if (game_state == RFID_SCANNED) {
    Serial.println(F("RFID_SCANNED"));
    //pulse the led to show we're waiting for input
    // TODO: reactivate the cancel button once we're not using it to get
    //       past the rfid check
    while(digitalRead(START_BUTTON) == LOW ) { //}&& digitalRead(CANCEL_BUTTON) == LOW) {
      float val = (exp(sin(millis()/1000.0*PI)) - 0.36787944)*108.0;
      analogWrite(START_LED, val);
    }
    delay(100);
    digitalWrite(START_LED, LOW);
    if (digitalRead(CANCEL_BUTTON) != HIGH) {
      game_state = COUNTDOWN;
    } else {
      game_state = IDLE;
    }
  } else if (game_state == COUNTDOWN) {
    Serial.print(F("COUNTDOWN"));
    // run through the countdown activities
    // this state is pretty time critical

    // reset all the game counters
    current_score = 0;
    update_scoreboard = false;
    take_photo = false;

    // then make the countdown
    myPacket.message_id = COUNT_FROM;
    myPacket.timestamp = 5000;
    radio.sendWithRetry(SCOREBD, (const void*)&myPacket, sizeof(myPacket), 5);

    start_time = millis() + (unsigned long)COUNT_DOWN;
    broadcastMessage(GAME_START);

    // wait for it…
    //while (start_time - millis() >= EARLY_PLAY);
    Serial.print(F("5… "));
    delay(1000);
    Serial.print(F("4… "));
    delay(1000);
    Serial.print(F("3… "));
    delay(1000);
    Serial.print(F("2… "));
    delay(1000);
    Serial.print(F("1… "));
    delay(1000);

    game_state = RUNNING;
    Serial.print(F("RUNNING"));
  } else if (game_state == RUNNING) {
    // lets' keep this fast, so score updates happen quickly
    checkTargets();

    if (update_scoreboard == true && millis() - start_time >= 0) {
      // there's been a new hit, send that
      scoreDisplay(current_score);
      Serial.println(current_score);
      update_scoreboard = false;
    }

    Serial.print(F("remaining time: "));
    Serial.println(RUN_TIMER - (millis() - start_time));

    if (millis() - start_time >= RUN_TIMER + GRACE_TIMER) {
      uint32_t number_of_hits = 0;
      for (int i=0; i<sizeof(hit_record); i++) {
         number_of_hits += hit_record[i];
      }
      if (current_score == 0 && number_of_hits == 0) {
        // there's been no impacts, allow some extra time
        myPacket.message_id = COUNT_FROM;
        myPacket.timestamp = EXTRA_TIMER;
        radio.sendWithRetry(TIMER, (const void*)&myPacket, sizeof(myPacket), 3);
        game_state = EXTRA_TIME;
        Serial.println(F("EXTRA_TIME"));
      } else {
        game_state = END_GAME;
      }
    }
  } else if (game_state == EXTRA_TIME) {
    // if there's been no score allow some extra time for a single hit
    checkTargets();
    if (update_scoreboard == true) {
      // just the one hit
      myPacket.score = current_score;
      uint32_t impact_num;
      for (int i=0; i<sizeof(hit_record); i++) {
        impact_num += hit_record[i];
      }
      myPacket.impact_num = impact_num;
      myPacket.timestamp = millis() - start_time;
      scoreDisplay(current_score);
      game_state = END_GAME;
      update_scoreboard = false;
    } else if (millis() - start_time >= RUN_TIMER + EXTRA_TIMER + GRACE_TIMER) {
      game_state = END_GAME;
    }
  } else if (game_state == END_GAME) {
    Serial.println(F("END_GAME"));
    // stop everything running
    myPacket.score = current_score;
    uint32_t impact_num;
    for (int i=0; i<sizeof(hit_record); i++) {
      impact_num += hit_record[i];
    }
    myPacket.impact_num = impact_num;
    myPacket.timestamp = millis() - start_time;
    broadcastMessage(GAME_END);

    // increment the counters for the next game
    game_uid++;

    game_state = IDLE;
  }

}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

void scoreDisplay(int32_t a_number) {
  myPacket.message_id = DISPLAY_NUM;
  myPacket.score = a_number;
  radio.sendWithRetry(SCOREBD, (const void*)&myPacket, sizeof(myPacket), 3);
}

void broadcastMessage(message_t the_message) {
  myPacket.message_id = the_message;
  myPacket.timestamp = millis() - start_time;
  radio.send(0xFF, (const void*)(&myPacket), sizeof(myPacket));
}

void checkTargets() {
  // check any incoming radio messages
  // *** We need to call this fast enough in-game ***
  if (radio.receiveDone()) {
    // check if we're getting hit data
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      if (theData.message_id == HIT) {
        uint8_t this_target = radio.SENDERID-1;
        if (this_target > NUM_TARGETS-1) { this_target = NUM_TARGETS-1; }
        uint8_t this_score = scoremap[this_target];
        if (game_state == COUNTDOWN) {
          current_score = current_score + (this_score*2);
        } else {
          current_score = current_score + (this_score);
        }
        hit_record[this_target] = theData.impact_num;
        update_scoreboard = true;
      }
    }
    if (radio.ACKRequested()) {
      uint8_t theNodeID = radio.SENDERID;
      radio.sendACK();
    }
    Blink(LED,3);
  }
}
