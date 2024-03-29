#include <SD.h>
#include <RFM69.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <avr/wdt.h>
#include <WirelessHEX69.h>
#include <Wire.h>
#include <SL018.h>
#include <RTClib.h>
#include <SOMO_II.h>
#include "constants.h"
#include "secrets.h"

SL018 rfid;
RFM69 radio;
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

// set the appropiate node ID
#define NODEID MAIN_CTRL    // The Gateway runs on node 200

// set up the somo-ii connection
SOMO_II mp3 = SOMO_II(Serial1);

// button and led connections
#define START_BUTTON  13
#define CANCEL_BUTTON 12
#define START_LED     15

// Lets get the global variables some space
Payload theData; // somewhere to store incoming data
Payload myPacket; // and one for oddutgoing
game_states_t game_state = IDLE;
char player_rfid[RFID_DIGITS+1];
long
  remaining_game_time = 0,
  current_score = 0;
unsigned long
  number_of_hits = 0,
  start_time = 0,
  game_uid = 0;
volatile bool
  update_scoreboard = false,
  take_photo = false;

// set the target scores:
long scoremap[] = { 101, 236, 553, 100, 1234, 5120, -100, 999, 1024, 512, 256, 128, 200, 865, 6547, 450 };
unsigned long hit_record[sizeof(scoremap)];

void setup() {
  // idiot pause on startup
  delay(2000);

  Wire.begin();
  Serial.begin(BAUD);
  Serial.println("starting made-invaders main_controller…");

  // initialise the music player
  Serial1.begin(SOMO_BAUD);
  mp3.begin();
  Serial.println(F("MP3 OK"));
  delay (1000);

  mp3.playFile(1, 6); //new start file
  Serial.println(F("Playing START.MP3"));

  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower();
#endif
  radio.encrypt(ENCRYPTKEY);

  // set up the button an led pins
  pinMode(START_LED, OUTPUT);
  pinMode(START_BUTTON, INPUT);
  pinMode(CANCEL_BUTTON, INPUT);

  // lets just prep the rfid array with zeros
  for (int i=0; i<RFID_DIGITS; i++) {
    myPacket.rfid_num[i] = '0';
  }

  Blink(START_LED, 1000);
  Serial.println(F("end of setup"));
}

uint16_t ackCount = 0;
void loop() {
  // start the state machine
  if (game_state == IDLE) {

    Serial.println(F("IDLE state"));
    start_time = millis();
    bool nothing_doing = false; // have we sent a "nothing doing" message
    // hold for rfid read
    Serial.println(F("waiting for rfid tag"));
    // we're waiting for an RFID scan
    rfid.seekTag();
    unsigned long play_repeat = 360000;
    unsigned long last_played_at = millis() - play_repeat + IDLE_TIMER;
    while (!rfid.available()) {
      if (nothing_doing == false) {
        if (millis() - start_time >= IDLE_TIMER) {
          Serial.println(F("nothing doing"));
          // broadcast NOTHING_DOING to go to idle mode
          myPacket.game_uid = 0;
          myPacket.impact_num = 0;
          myPacket.score = 0;
          for (int i=0; i<RFID_DIGITS; i++) {
            myPacket.rfid_num[i] = '0';
          }
          myPacket.timestamp = 0;
          myPacket.message_id = NOTHING_DOING;
          broadcastMessage(NOTHING_DOING);
          nothing_doing = true;
        }
      } else {
        if (millis() - last_played_at >= play_repeat) {
          // Play the idle theme
          Serial.println(F("Playing Ed's music that sounds like donkey kong country"));
          mp3.playFile(1,9);
          last_played_at = millis();
        }
      }
    }
    strncpy(player_rfid, rfid.getTagString(), sizeof(player_rfid));
    // then change this to lower case because Brett insisted
    for (int i=0; i<sizeof(player_rfid); i++) {
      player_rfid[i] = tolower(player_rfid[i]);
    }
    // and print to serial so we can see too
    Serial.print(F("rfid: "));
    Serial.println(player_rfid);
    // copy all the game variables into the right places for sending
    strncpy(myPacket.rfid_num, player_rfid, sizeof(myPacket.rfid_num));
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
    mp3.playFile(1,7);
    //pulse the led to show we're waiting for input
    int start_button_state = LOW;
    int cancel_button_state = LOW;
    while(start_button_state == LOW && cancel_button_state == LOW) {
      float val = (exp(sin(millis()/1000.0*PI)) - 0.36787944)*108.0;
      analogWrite(START_LED, val);
      start_button_state = digitalRead(START_BUTTON);
      cancel_button_state = digitalRead(CANCEL_BUTTON);
    }
    delay(100);
    digitalWrite(START_LED, LOW);
    if (start_button_state == HIGH) {
      game_state = COUNTDOWN;
    } else {
      broadcastMessage(CANCEL_GAME);
      game_state = IDLE;
    }
  } else if (game_state == COUNTDOWN) {
    Serial.println(F("COUNTDOWN"));
    // run through the countdown activities
    // this state is pretty time critical

    // reset all the game counters
    current_score = 0;
    number_of_hits = 0;
    for (int i=0; i<sizeof(scoremap); i++) {
       hit_record[i] = 0;
    }
    update_scoreboard = true; // make sure we get an inital refresh to zero
    take_photo = true;

    // then make the countdown
    myPacket.message_id = RUN_COUNTDOWN;
    myPacket.score = 5;
    radio.sendWithRetry(SCOREBD, (const void*)&myPacket, sizeof(myPacket), 5);
    //delay(50);
    Serial.print(F("5… "));
    mp3.playFile(1,5);
    long countdown_timer = millis();
    while (millis() - countdown_timer <= 1000UL);
    mp3.stop();
    mp3.playFile(1,4);
    Serial.print(F("4… "));
    while (millis() - countdown_timer <= 2000UL);
    mp3.stop();
    delay(10);
    mp3.playFile(1,3);
    Serial.print(F("3… "));
    while (millis() - countdown_timer <= 3000UL);
    mp3.stop();
    delay(10);
    mp3.playFile(1,2);
    Serial.print(F("2… "));
    myPacket.message_id = GAME_START;
    myPacket.timestamp = -1500L;
    // broadcast game start
    radio.send(0xFF, (const void*)&myPacket, sizeof(myPacket));
    while (millis() - countdown_timer <= 4000UL) {
      checkTargets();
    }
    Serial.print(F("1… "));
    mp3.stop();
    delay(10);
    mp3.playFile(1,2);
    while(millis() - countdown_timer <= 5000UL);
    game_state = RUNNING;
    Serial.println(F("RUNNING"));
    mp3.stop();
    delay(10);
    mp3.playFile(1,6);
    // send a directed start message to begin the timer
    start_time = millis();
    myPacket.message_id = GAME_START;
    //radio.sendWithRetry(TIMER, (const void*)&myPacket, sizeof(myPacket), 3);
    radio.send(TIMER, (const void*)&myPacket, sizeof(myPacket));
    while (millis() - countdown_timer <= 6000UL);
    mp3.stop();
    delay(10);
    mp3.playFile(1,random(10, 13));

  } else if (game_state == RUNNING) {
    // lets' keep this fast, so score updates happen quickly
    checkTargets();

    if (update_scoreboard == true && millis() - start_time >= 0) {
      // there's been a new hit, send that
      scoreDisplay(current_score);
      Serial.println(current_score);
      update_scoreboard = false;
    }

    if (take_photo == true) {
      remaining_game_time = (long)(RUN_TIMER - (millis() - start_time));
      if (remaining_game_time <= RUN_TIMER/2) {
        Serial.print(F("take a photo with "));
        Serial.print(remaining_game_time/1000);
        Serial.println(F(" seconds remaining"));
        myPacket.message_id = TAKE_PHOTO;
        radio.send(REG_STN, (const void*)&myPacket, sizeof(myPacket));
        take_photo = false;
      }
    }

    if (millis() - start_time >= RUN_TIMER + GRACE_TIMER) {
      for (int i=0; i<sizeof(scoremap); i++) {
         number_of_hits += hit_record[i];
      }
      if (current_score == 0 && number_of_hits == 0) {
        mp3.stop();
        delay(10);
        mp3.playFile(1,9);
        // there's been no impacts, allow some extra time
        myPacket.message_id = MORE_TIME;
        radio.sendWithRetry(TIMER, (const void*)&myPacket, sizeof(myPacket), 3);
        game_state = EXTRA_TIME;
        Serial.println(F("EXTRA_TIME"));
      } else {
        Serial.print(F("end score: "));
        Serial.println(current_score);
        Serial.print(F("number of hits: "));
        Serial.println(number_of_hits);
        game_state = END_GAME;
      }
    }
  } else if (game_state == EXTRA_TIME) {
    // if there's been no score allow some extra time for a single hit
    checkTargets();

    if (update_scoreboard == true) {
      // just the one hit
      scoreDisplay(current_score);
      Serial.print(F("end score: "));
      Serial.println(current_score);
      Serial.print(F("number of hits: "));
      Serial.print(number_of_hits);
      game_state = END_GAME;
      update_scoreboard = false;
    } else if (millis() - start_time >= RUN_TIMER + EXTRA_TIMER + GRACE_TIMER) {
      scoreDisplay(current_score);
      Serial.print(F("end score: "));
      Serial.println(current_score);
      Serial.print(F("number of hits: "));
      Serial.print(number_of_hits);
      game_state = END_GAME;
    //} else {
      //Serial.println(millis()-start_time);
    } else {
      delay(250);
    }
  } else if (game_state == END_GAME) {
    Serial.println(F("END_GAME"));
    // Play the end game sound
    mp3.playFile(1, 8);
    // stop everything running
    myPacket.score = current_score;
    for (int i=0; i<sizeof(scoremap); i++) {
      number_of_hits += hit_record[i];
    }
    myPacket.impact_num = number_of_hits;
    myPacket.timestamp = millis() - start_time;
    scoreDisplay(current_score);
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
  Serial.print(F("display score: "));
  Serial.println(myPacket.score);
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
    Blink(START_LED,100);
  }
}
