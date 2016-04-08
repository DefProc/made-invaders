/* invader display

   take the invader image from memory and

*/

#include "FastLED.h"

#define BUFFPIXEL 1

// How many leds in your strip?
#define NUM_LEDS 81

// set the WS2182b data pin
#define DATA_PIN 6

// Define the array of leds
CRGB leds[NUM_LEDS];

// The invader logo (9×9 leds)
const uint8_t invader[NUM_LEDS][3] = {
  {46,10,4},{46,10,4},{130,28,11},{110,23,9},{93,20,8},{80,37,67},{71,68,166},{56,123,196},{62,198,204},
  {47,127,212},{79,70,166},{128,53,88},{255,255,255},{43,9,3},{255,255,255},{147,63,12},{48,97,3},{0,0,0},
  {46,137,3},{91,103,104},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{128,27,11},{30,38,99},
  {93,20,8},{58,12,5},{255,255,255},{92,51,7},{255,255,255},{0,164,0},{255,255,255},{118,60,115},{22,4,1},
  {74,15,5},{255,255,255},{255,255,255},{255,255,255},{255,255,255},{255,255,255 },{255,255,255},{255,255,255},{0,0,0},
  {46,10,4},{255,255,255},{40,89,63},{255,255,255},{124,79,32},{255,255,255},{128,96,63},{255,255,255},{63,45,4},
  {57,56,64},{255,255,255},{27,170,98},{255,255,255},{64,82,156},{255,255,255},{69,69,38},{255,255,255},{69,72,179},
  {114,44,69},{76,64,150},{64,73,148},{88,50,6},{58,12,5},{101,21,8},{153,32,12},{116,25,10},{0,0,0},
  {0,0,0},{162,35,14},{106,22,8},{58,12,5},{58,12,5},{101,21,8},{101,21,8},{22,4,1},{22,4,1}
};

void setup() {
  Serial.begin(9600);
  Serial.println("start bmp_reader");
  FastLED.addLeds<WS2812,DATA_PIN,RGB>(leds,NUM_LEDS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(100);

  for (int i=0; i<NUM_LEDS; i++) {
    leds[i].r = invader[i][0];
    leds[i].g = invader[i][1];
    leds[i].b = invader[i][2];
  }
  FastLED.show();
}


void loop() {}
