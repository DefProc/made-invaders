/* Serial controlled countdown timer and integer display (scoreboard) for bit.ly/big-7-seg displays.

  Accepted input
    H - display accepted inputs
    T<nnn> - set timer lnength
    S - start
    R - reset the time (call before Start)
    X - stop the current coutdown
    Z - zero the timer
    D<nnn> - display an integer

*/

#include <RFM69.h>
#include <SPI.h>
#include "constants.h"
#include "secrets.h"

//#define NODEID TIMER
#define NODEID SCOREBD
RFM69 radio;
Payload theData;

#define LE 3 //latch
#define NOE 4 //pwm
#define CLK 5 //clock
#define SDO 6 //serial data

#define TIMER_DEFAULT 300 // 30.0 seconds
#define TIMER_RESOLUTION 10 // 10 ms = 0.01 sec
#define HOLD_TIME 1000 // ms

#if NODEID == SCOREBD
#define DIGITS 6
#define DECIMAL_PLACES 0
#else
#define DIGITS 3
#define DECIMAL_PLACES 1
#endif

byte segments[] =
  {
    0b11111100, //0
    0b01100000, //1
    0b11011010, //2
    0b11110010, //3
    0b01100110, //4
    0b10110110, //5
    0b10111110, //6
    0b11100000, //7
    0b11111110, //8
    0b11100110, //9
    0b00000001, //dot (POINT)
    0b00000000, //off (BLANK)
    0b00000010, //-   (MINUS)
  };

#define POINT 10
#define BLANK 11
#define MINUS 12

bool run_flag = false;
bool ready_flag = false;
unsigned long counter = 0UL;
long timer = TIMER_DEFAULT;

// prototype for defalt decimal places for display
void displayTime(long number, int decimalplaces = DECIMAL_PLACES);

void setup() {
  Serial.begin(BAUD);
  pinMode(LE,OUTPUT);
  pinMode(NOE,OUTPUT);
  digitalWrite(NOE,LOW);
  pinMode(CLK,OUTPUT);
  pinMode(SDO,OUTPUT);

  digitalWrite(LE,false);

  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower();
#endif
  radio.encrypt(ENCRYPTKEY);

  Serial.println(F("Coutdown Timer and Integer Display"));
  Serial.println(F("=================================="));

  displayTime(0);
}

void loop() {
  // if there's in incoming radio instruction, use that
  checkIncoming();

  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    char character = Serial.read();

    if (character == 'h' || character == 'H') {
      Serial.println(F("Accepted input"));
      Serial.println(F("  H - display this message"));
      Serial.println(F("  S - start"));
      Serial.println(F("  T<nnnn> - set the timer length (1/10 sec)"));
      Serial.println(F("  R - reset the time (call before Start)"));
      Serial.println(F("  X - stop the current coutdown"));
      Serial.println(F("  Z - zero the timer"));
      Serial.println(F("  D<nnnn> - display (integer)"));
      Serial.println(F("  C<n> - countdown timer (seconds)"));
    }

    // parse any incoming commands
    if (character == 'S' || character == 's') {
      displayTime(timer);

      run_flag = true;
      counter = millis();
    }

    if (character == 'T' || character == 't') {
      timer = Serial.parseInt();
    }

    if (character == 'r' || character == 'R') {
      run_flag = false;
      displayTime(timer);
    }

    if (character == 'x' || character == 'X') {
      run_flag = false;
    }

    if (character == 'z' || character == '|') {
      run_flag = false;
      displayTime(0);
    }

    if (character == 'D' || character == 'd') {
      long number = Serial.parseInt();
      displayTime(number, 0);
    }

    if (character == 'C' || character == 'c') {
      long number = abs(Serial.parseInt()) * 10;
      run_flag = true;
    }
  }

  // update the time each time through the loop, if the timer is running
  if (run_flag == true) {
    if (millis() - counter < timer * 100) {
      int time_to_display = timer - ((millis() - counter)/100);
      displayTime(time_to_display);
      //if (time_to_display % 10 == 0) {
        //Serial.println(time_to_display);
      //}
    } else {
      displayTime(0);
      run_flag = false;
    }
  }
}


void displayTime(long number, int decimal_places) {
  boolean negative = false;
  digitalWrite(LE,LOW);
  // calculate each digit for sending
  if (number < 0) {
    negative=true;
    number = abs(number);
  }
  for (byte i=0; i<DIGITS; i++) {
    byte output = number % 10;
    if (i == decimal_places && i != 0) {
      // add decimal place if specified digit
      // but ignore for whole numbers
      shiftOut(SDO, CLK, LSBFIRST, segments[output] | segments[POINT]);
    } else if (output == 0 && number == 0 && i != 0) {
      if (negative == true) {
        // print a minus if we're at the first negative
        // (assumes we won't see max length number as a minus)
        shiftOut(SDO, CLK, LSBFIRST, segments[MINUS]);
        negative=false; //clear the flag to make minuses
      } else {
        shiftOut(SDO, CLK, LSBFIRST, segments[BLANK]);
      }
    } else {
      // display the digit
      shiftOut(SDO, CLK, LSBFIRST, segments[output]);
    }
    number = number / 10;
  }
  digitalWrite(LE,HIGH);
}

void checkIncoming() {
  // check any incoming radio messages
  // *** We need to call this fast enough so as humans don't notice ***
  if (radio.receiveDone()) {
    // check if we're getting hit data
    if (radio.DATALEN = sizeof(Payload)) {
      theData = *(Payload*)radio.DATA;
      switch (theData.message_id) {
        case GET_READY:
          Serial.println(F("GET_READY"));
          if (NODEID == TIMER) {
            run_flag = false;
            timer = RUN_TIMER/100;
            displayTime(timer);
          } else {
            // we aren't a timer, just a display
            run_flag = false;
            displayTime(0, 0);
          }
          break;
        case CANCEL_GAME:
          Serial.println(F("CANCEL"));
          run_flag = false;
          if (NODEID == TIMER) {
            displayTime(0);
          } else {
            displayTime(0, 0);
          }
          break;
        case GAME_START:
          Serial.print(F("GAME_START "));
          if (radio.TARGETID == NODEID) {
            // only start the timer if it's addressed to us
            // so this will ignore a broadcast GAME_START command
            Serial.println(F("listened"));
            displayTime(timer);
            run_flag = true;
            counter = millis();
          } else {
            Serial.println(F("ignored"));
            Serial.print(F("sent to "));
            Serial.println(radio.TARGETID);
          }
          break;
        case MORE_TIME:
          if (NODEID == TIMER) {
            Serial.println(F("MORE_TIME"));
            run_flag = true;
            timer = EXTRA_TIMER/100;
            displayTime(timer);
            counter = millis();
          }
          break;
        case GAME_END:
          Serial.println(F("GAME_END"));
          run_flag = false;
          ready_flag = false;
          if (NODEID == TIMER) {
            displayTime(0);
          }
          break;
        case DISPLAY_NUM:
          Serial.print(F("DISPLAY_NUM: "));
          Serial.print(theData.score);
          run_flag = false;
          displayTime(theData.score, 0);
          break;
        case RUN_COUNTDOWN:
          Serial.print(F("RUN_COUNTDOWN"));
          timer = abs(theData.score) * 10;
          Serial.println(theData.score);
          run_flag = true;
          break;
      }
    }
    if (radio.ACKRequested()) {
      uint8_t theNodeID = radio.SENDERID;
      radio.sendACK();
    }
  }
}
