#include <SPI.h>
#include <RFM69.h>
#include "constants.h"
#include "secrets.h"

RFM69 radio;

// set the appropiate node ID
#define NODEID MAIN_CTRL    // The Gateway runs on node 200

// button and led connections
#define START_BUTTON  13
#define CANCEL_BUTTON 12

// Lets get the global variables some space
Payload theData; // somewhere to store incoming data
Payload myPacket; // and one for oddutgoing
game_states_t game_state = IDLE;
long
  current_score = 0;
unsigned long
  number_of_hits = 0,
  start_time = 0,
  game_uid = 0;
volatile bool
  update_scoreboard = false;

// set the target scores:
long scoremap[] = { 101, 236, 553, 100, 1234, 5120, -100, 999, 1024, 512, 256, 128, 200, 865, 6547, 450 };
unsigned long hit_record[sizeof(scoremap)];

void setup() {
  // idiot pause on startup
  delay(1000);

  Serial.begin(BAUD);
  Serial.println("starting made-invaders dummy_main…");

  // initialize the radio
  if (!radio.initialize(FREQUENCY,NODEID,NETWORKID)) {
    // the radio doesn't exist
    Serial.println(F("no radio"));
    while(1);
  } else {
    Serial.println(F("radio found"));
  }
#ifdef IS_RFM69HW
  Serial.println(F("using high power radio mode"));
  radio.setHighPower();
#endif
  radio.encrypt(ENCRYPTKEY);

  Serial.println(F("end of setup"));
}

uint16_t ackCount = 0;
void loop() {
  // start the state machine
  if (game_state == IDLE) {
    Serial.println(F("IDLE state"));
    // copy all the game variables into the right places for sending
    current_score = 0;
    myPacket.game_uid = game_uid;
    myPacket.impact_num = 0;
    myPacket.score = current_score;
    myPacket.timestamp = 30000;
    // let everybody know what's happening
    Serial.println(F("send scoreboardDisplay(current_score)"));
    scoreDisplay(current_score);
    Serial.println(F("broadcast GET_READY"));
    broadcastMessage(GET_READY);
    // lets at least try to make sure the registration station gets this
    //radio.sendWithRetry(REG_STN, (const void*)&myPacket, sizeof(myPacket), 5);
    Serial.println(F("Ready to play (press \'S\')"));
    game_state = RFID_SCANNED;
  } else if (game_state == RUNNING) {
    // lets' keep this fast, so score updates happen quickly
    checkTargets();

    if (update_scoreboard == true) {
      // there's been a new hit, send that
      scoreDisplay(current_score);
      Serial.println(current_score);
      update_scoreboard = false;
    }

  } else if (game_state == END_GAME) {
    Serial.println(F("END_GAME"));
    // stop everything running
    myPacket.score = current_score;
    for (int i=0; i<sizeof(scoremap); i++) {
      number_of_hits += hit_record[i];
    }
    myPacket.impact_num = number_of_hits;
    Serial.print(F("number of hits: "));
    Serial.println(number_of_hits);
    myPacket.timestamp = millis() - start_time;
    Serial.println(F("broadcast game end"));
    broadcastMessage(GAME_END);
    Serial.println(F("display final score"));
    scoreDisplay(current_score);

    // increment the counters for the next game
    game_uid++;

    game_state = RFID_SCANNED;
    Serial.println(F("Game stopped (press \'I\' to go to idle)"));
   }

  if (Serial.available() > 0) {
    char sue = Serial.read();
    if (sue == 'h' || sue == 'H') {
      // print out the help message
      Serial.println(F("made-invaders dummy_main controller"));
      Serial.println(F("I - Idle mode (prep for next game)"));
      Serial.println(F("S - Start game"));
      Serial.println(F("X - Stop game (and go to idle)"));
    } else if (sue == 'i' || sue == 'I') {
      // idle mode
      game_state = IDLE;
    } else if (sue == 's' || sue == 'S') {
      // start game
      broadcastMessage(GAME_START);
      game_state = RUNNING;
      Serial.println(F("Game started (press \'X\' to stop)"));
    } else if (sue == 'x' || sue == 'X') {
      // stop game
      game_state = END_GAME;
    }
    while (Serial.available() > 0) {
      // flush the serial buffer
      Serial.read();
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

void scoreDisplay(int32_t a_number) {
  Payload aPacket;
  aPacket.message_id = DISPLAY_NUM;
  aPacket.score = a_number;
  if (radio.sendWithRetry(SCOREBD, (const void*)&aPacket, sizeof(aPacket), 2, 50)) {
    Serial.print(F("display score: "));
    Serial.println(myPacket.score);
  } else {
    Serial.println(F("No ACK from scoreboard"));
  }
}

void broadcastMessage(message_t the_message) {
  myPacket.message_id = the_message;
  myPacket.timestamp = millis() - start_time;
  radio.send(0xFF, (const void*)(&myPacket), sizeof(myPacket), false);
  Serial.print(F("broadcast message: "));
  Serial.println(the_message);
}

void checkTargets() {
  // check any incoming radio messages
  // *** We need to call this fast enough in-game ***
  if (radio.receiveDone()) {
    // check if we're getting hit data
    if (radio.ACKRequested() && radio.SENDERID <= NUM_TARGETS) {
      //uint8_t theNodeID = radio.SENDERID;
      radio.sendACK();
      Serial.print(F("ACK "));
      Serial.println(radio.SENDERID);
    }
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      if (theData.message_id == HIT) {
        uint8_t this_target = radio.SENDERID-1;
        Serial.print(F("HIT: "));
        Serial.print(this_target);
        Serial.print(F(" SCORED: "));
        Serial.println(scoremap[this_target]);
        if (this_target >= NUM_TARGETS) { this_target = NUM_TARGETS-1; }
        long this_score = scoremap[this_target];
        if (game_state == COUNTDOWN) {
          current_score = current_score + (this_score*2);
        } else {
          current_score = current_score + (this_score);
        }
        hit_record[this_target] = theData.impact_num;
        update_scoreboard = true;
      }
    }
  }
}

void radioAcks() {
  // check any incoming radio messages
  // *** We need to call this fast enough in-game ***
  if (radio.receiveDone()) {
    // check if we're getting hit data
    radio.sendACK();
    Serial.print(F("ACK "));
    Serial.println(radio.SENDERID);
  }
}
