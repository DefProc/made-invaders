#include "FastLED.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// RGB Calibration code
// in addition to checking the LED colour ordering (expected GRB), this test allows checking the
// 1m long strips of 30 LEDs, prior to wiring them up in to the LEDframes.

// You should see six leds on.  If the RGB ordering is correct, you should see 1 red led, 2 green
// leds, and 3 blue leds.  If you see different colors, the count of each color tells you what the
// position for that color in the rgb orering should be.  So, for example, if you see 1 Blue, and 2
// Red, and 3 Green leds then the rgb ordering should be BRG (Blue, Red, Green).

//////////////////////////////////////////////////

#define NUM_LEDS 30

// Data pin that led data will be written out over
#define DATA_PIN 6
// Clock pin only needed for SPI based chipsets when not using hardware SPI
//#define CLOCK_PIN 8

CRGB leds[NUM_LEDS];

void setup() {
	// sanity check delay - allows reprogramming if accidently blowing power w/leds
 	delay(2000);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  // set the first 6 leds to R,G,G,B,B,B
  leds[0] = CRGB(255,0,0);
  leds[1] = CRGB(0,255,0);
  leds[2] = CRGB(0,255,0);
  leds[3] = CRGB(0,0,255);
  leds[4] = CRGB(0,0,255);
  leds[5] = CRGB(0,0,255);
  for (uint8_t i=6; i<NUM_LEDS; i++) {
    leds[i] = CRGB(0,0,0);
  }
  FastLED.show();
  delay(2000);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void loop() {
  // show the reds one LED at a time
  for (uint8_t i=0; i < NUM_LEDS+5; i++) {
    if (i<NUM_LEDS) {
      leds[i] = CRGB::Red;
    }
    FastLED.show();
    if (i >= 5) {
      leds[i-5] = CRGB::Black;
    }
    delay(50);
  }
  delay(300);
  // show the green one LED at a time
  for (uint8_t i=0; i < NUM_LEDS+5; i++) {
    if (i<NUM_LEDS) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
    if (i >= 5) {
      leds[i-5] = CRGB::Black;
    }
    delay(50);
  }
  delay(300);
  // show the blue one LED at a time
  for (uint8_t i=0; i < NUM_LEDS+5; i++) {
    if (i<NUM_LEDS) {
      leds[i] = CRGB::Blue;
    }
    FastLED.show();
    if (i >= 5) {
      leds[i-5] = CRGB::Black;
    }
    delay(50);
  }
  delay(300);
}
