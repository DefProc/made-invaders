#include <SPI.h>
#include <RFM69.h>
#include "constants.h"
#include "secrets.h"

// create the radio object
RFM69 radio;

// set the appropiate node ID
#define NODEID MAIN_CTRL    // The Gateway runs on node 200

// define some contsants for automatic play
#define PLAY_TIME 30000UL
#define WAIT_TIME 60000UL

// Lets get the global variables some space
Payload theData; // somewhere to store any incoming data
Payload myPacket; // and one for outgoing radio data
game_states_t game_state = IDLE; // hold the state for game operation
bool play_auto = true; // should we just run automatically?
long current_score = 0; // variable to hold the current score
unsigned long number_of_hits = 0; // we also hold the total number of target hits
unsigned long start_time = 0; // the millisecond register of the game start time
unsigned long game_uid = 0; // unique game identifier
volatile bool update_scoreboard = false; // do we need to update the scoreboard

// hold an array of the target scores:
long scoremap[] = { 101, 236, 553, 100, 1234, 5120, -100, 999, 1024, 512, 256, 128, 200, 865, 6547, 450 };
// and an array the same size for the number of hits per target
unsigned long hit_record[sizeof(scoremap)];

void setup() {
  // idiot pause on startup
  delay(1000);

  // start the serial connection and print the start program message
  Serial.begin(BAUD);
  Serial.println("starting made-invaders dummy_main…");

  // initialize the radio
  Serial.print(F("Radio…"));
  if (!radio.initialize(FREQUENCY,NODEID,NETWORKID)) {
    // the radio module isn't there!
    Serial.println(F("FAIL"));
    while(1);
  }
  Serial.println(F("OK"));

#ifdef IS_RFM69HW
  Serial.println(F("Radio HIGH POWER"));
  radio.setHighPower();
#endif

  // set the encryption key for the radio coms (defined in the secrets.h file)
  radio.encrypt(ENCRYPTKEY);

  Serial.println(F("setup END"));
}

uint16_t ackCount = 0;
void loop() {
  // start the state machine
  if (game_state == IDLE) {
    Serial.println(F("GAME_RESET"));
    // copy all the game variables into the right places for sending
    current_score = 0;
    myPacket.game_uid = game_uid;
    myPacket.impact_num = 0;
    myPacket.score = current_score;
    myPacket.timestamp = 30000;
    // update the scoreboard with the new zero score
    Serial.println(F("display zero score"));
    scoreDisplay(current_score);
    // then send a broadcast message (to every receiver) to get ready
    Serial.println(F("broadcast GET_READY"));
    broadcastMessage(GET_READY);

    Serial.println(F("Ready to play (press \'S\')"));
    // move out of this game state, so we don't do this again
    start_time = millis();
    game_state = COUNTDOWN;
  } else if (game_state == RUNNING) {
    // lets' keep this fast, so score updates happen quickly
    checkTargets();

    if (update_scoreboard == true) {
      // there's been a new hit, send that to the scoreboard
      scoreDisplay(current_score);
      Serial.print(F("SCORE: "));
      Serial.println(current_score);
      // and then don't send it again
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

    // change state so we don't do this again
    game_state = EXTRA_TIME;
    Serial.println(F("Game stopped (press \'R\' to reset)"));
  }

  // check if we should be automatically starting to play
  if (play_auto == true) {
    // cycle the play state if we're running in auto mode
    if (game_state == EXTRA_TIME && millis() - start_time >= PLAY_TIME + WAIT_TIME) {
      // we've been stopped for more than 60 seconds
      Serial.print(F("AUTO START"));
      game_state = IDLE;
    } else if (game_state == COUNTDOWN) {
      // we've just gone to idle from auto start
      Serial.println(F("GO!!!"));
      game_state = RUNNING;
    } else if (game_state == RUNNING && millis() - start_time >= PLAY_TIME) {
      Serial.println(F("AUTO STOP"));
      game_state = END_GAME;
    }
  }

  // every time through the loop, we'll check for serial instructions
  if (Serial.available() > 0) {
    // read the incoming character (char)
    char sue = Serial.read();
    // check if it matches any of our accepted instructions (H,R,S,X)
    if (sue == 'h' || sue == 'H') {
      // print out the help message
      Serial.println(F("made-invaders dummy_main controller"));
      Serial.println(F("R - Reset"));
      Serial.println(F("S - Start game"));
      Serial.println(F("X - Stop game"));
      Serial.println(F("A - run auto (30s games, 60s apart)"));
    } else if (sue == 'r' || sue == 'R') {
      // reset the game
      game_state = IDLE;
      play_auto = false;
    } else if (sue == 's' || sue == 'S') {
      // start game
      broadcastMessage(GAME_START);
      game_state = RUNNING;
      play_auto = false;
      Serial.println(F("Game started (press \'X\' to stop)"));
    } else if (sue == 'x' || sue == 'X') {
      // stop game
      game_state = END_GAME;
      play_auto = false;
    } else if (sue == 'a' || sue == 'A') {
      game_state = IDLE;
      play_auto = true;
      Serial.println(F("AUTO PLAY ON"));
    }
    while (Serial.available() > 0) {
      // flush any remaining character in the serial buffer
      // as we only care about single character messages
      Serial.read();
    }
  }
}

// a helper function to make it easier to send numbers to the scoreboard
void scoreDisplay(int32_t a_number) {
  Payload aPacket;
  aPacket.message_id = DISPLAY_NUM;
  aPacket.score = a_number;
  if (radio.sendWithRetry(SCOREBD, (const void*)&aPacket, sizeof(aPacket), 2, 50)) {
    Serial.print(F("Display: "));
    Serial.println(myPacket.score);
  } else {
    Serial.println(F("No response (ACK) from scoreboard"));
  }
}

// a helper function to send broadcast messages
void broadcastMessage(message_t the_message) {
  myPacket.message_id = the_message;
  myPacket.timestamp = millis() - start_time;
  radio.send(0xFF, (const void*)(&myPacket), sizeof(myPacket), false);
  Serial.print(F("broadcast message: "));
  Serial.println(the_message);
}

// check incoming messages while the game is running
void checkTargets() {
  // check any incoming radio messages
  // *** We need to call this fast enough in-game that we don't miss any***
  if (radio.receiveDone()) {
    // check if we're getting hit data
    if (radio.ACKRequested() && radio.SENDERID <= NUM_TARGETS) {
      // send an ACK (acknowledgement) straight away if requested
      radio.sendACK();
      Serial.print(F("ACK:"));
      Serial.println(radio.SENDERID);
    }
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      if (theData.message_id == HIT) {
        // the target radio ids start at 1
        // but the target arrays are zero indexed
        uint8_t this_target = radio.SENDERID-1;
        // print the message to serial so we can see
        Serial.print(F("HIT: "));
        Serial.print(this_target);
        Serial.print(F(" SCORED: "));
        Serial.println(scoremap[this_target]);

        if (this_target <= NUM_TARGETS) {
          // add the target score to the current score
          current_score = current_score + (scoremap[this_target]);
          // set the hit counter for the target to the incoming value
          hit_record[this_target] = theData.impact_num;
          // set the flag so the scoreboard will be updated
          update_scoreboard = true;
        }
      }
    }
  }
}

void radioAcks() {
  // automatically ACK any messages
  if (radio.receiveDone()) {
    // check if we're getting hit data
    radio.sendACK();
    Serial.print(F("ACK "));
    Serial.println(radio.SENDERID);
  }
}
