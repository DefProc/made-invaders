#include "FastLED.h"
#include <SdFat.h>
#include <TimerOne.h>
#include <RFM69.h>
#include <SPI.h>
#include "constants.h"
#include "secrets.h"
SdFat sd;
SdFile myFile;

// The unique identifier of this node
// targets are numbered 1-16,
#define NODEID 1
RFM69 radio;

Payload myPacket;

// How many numbered images on the SD card?
#define NUM_IMAGES 7

#define BUFFPIXEL 20
#define NUM_LEDS 90
#define ARRAY_WIDTH 9
#define ARRAY_HEIGHT 9
#define DATA_PIN 6
// set the inital (default) frame hold time
#define FRAME_DELAY 250
// Define the array of leds
CRGB leds[NUM_LEDS];
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
// timer frequency for the ADC
const unsigned char PS_16 = (1 << ADPS2);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

// the machine states
enum play_state_t {
  IDLE,
  READY,
  SUPER_SECRET,
  PLAY,
  GAME_OVER,
  TEST
};

// possible image states
enum image_state_t {
  STATIC,
  VERTICAL,
  HORIZONTAL
};

// System Setup
volatile boolean
  changeImage = false; // should we change the image to the next one?
boolean
  singleGraphic = false, // single BMP file
  setupActive = false; // set brightness, playback mode, etc.
int
  offsetX = 0, // for translating images x pixels
  offsetY = 0; // for translating images y pixels
uint8_t
  analogThreshold = 70, // above what value should we take a reading as activated
  currentFrame = 0, // whick frame are we displaying
  numFrames = 1, // how many frames in the image animation?
  currentImage = -1; // which image are we looking at
uint16_t
  imageWidth = 0,
  imageHeight = 0;
uint32_t
  gameStartTime = 0UL,
  frameDelay = FRAME_DELAY, // the time to hold each animation frame
  lastImageUpdate = 0UL, // when did we last change the displayed image
  impactRepeat = 100UL; // debounce timeout for the sensor
volatile uint32_t
  lastImpact = 0UL; // when did we last see a hit
play_state_t play_state = TEST;
image_state_t image_state = STATIC;

int showImage(char* filename, uint32_t frame_delay = frameDelay, uint8_t first_frame = currentFrame, uint16_t num_frames = 0, uint16_t repeat_number = 1);

void setup() {
  Serial.begin(115200);
  Serial.println("starting radio_test_send");

  // set up the ADC sampling speed
  ADCSRA &= ~PS_128;  // remove bits set by Arduino library
  ADCSRA |= PS_16;    // set the prescaler to 16 (1MHz)

  // initialize the LED string
  FastLED.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
  FastLED.setBrightness(84);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // start the SD card
  if (!sd.begin(4, SPI_HALF_SPEED)) {
    // display an error pattern on SD fail
    for (uint8_t i=0; i<NUM_LEDS; i++) {
      if (i % 2 == 0) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Black;
      }
    }
    FastLED.show();
    sd.initErrorHalt();
  } else {
    Serial.println(F("SD began"));
    //sd.ls(LS_R);
  }

  // initialize the radio
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower(); //uncomment only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);

  // show the colour wipe, so we know the display's working
  colour_wipe();

  // draw the invader logo
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(100);
  showImage("invader.bmp");
  delay(1000);

  if (play_state == TEST) {
    startScoring();
  }
}

void startScoring() {
  // Start the fast interrupt for the analog read
  pinMode(A0, INPUT);
  // TODO: include the analog input for more reliable scoring
  Timer1.initialize(10000); // run at 100Hz
  Timer1.attachInterrupt(checkImpact);

  // and start watching INT1 for too
  pinMode(3, INPUT);
  attachInterrupt(INT1, piezoInt, FALLING);

  gameStartTime = millis() - 5000UL;
}

void stopScoring() {
  Timer1.detachInterrupt();
  detachInterrupt(INT1);
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } }

void loop() {
  if (play_state == TEST) {
    if (changeImage == true) {
      char filename_buffer[13];
      currentImage++;
      currentImage = currentImage % (NUM_IMAGES);
      sprintf(filename_buffer, "%04d.bmp", currentImage+1);
      if (sd.exists(filename_buffer)) {
        // change the image (ASAP)
        Serial.print("Displaying ");
        Serial.println(filename_buffer);
        // show the first frame (if it's an animation)
        showImage(filename_buffer, 0, 0, 1);
        changeImage = false;
        // Then finally report the hit via the RFM69
        myPacket.message_id = HIT;
        myPacket.impact_num++;
        myPacket.timestamp = millis() - gameStartTime;
        if (radio.sendWithRetry(MAIN_CTRL, (const void*)(&myPacket), sizeof(myPacket), 5)) {
          Serial.println(F("sent impact message"));
        } else {
          Serial.println(F("no radio message sent"));
        }

      } else {
        Serial.print(filename_buffer);
        Serial.println(" doesn't exist");
      }
    } else if (image_state != STATIC && millis() - lastImageUpdate >= frameDelay) {
      // we should update the current image if it's an animation at it's time to
      char filename_buffer[13];
      sprintf(filename_buffer, "%04d.bmp", currentImage+1);
      // we've already checked that this exists
      showImage(filename_buffer, 0, currentFrame, 1);
    }
  }
}

void colour_wipe() {
  static uint8_t hue = 0;
  Serial.print("x");
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    //now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  Serial.print("x");

  // Now go in the other direction.
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}

void checkImpact() {
  if (analogRead(A0) >= analogThreshold && millis() - lastImpact >= impactRepeat) {
    changeImage = true;
    lastImpact = millis();
  }
}

void piezoInt() {
  if (millis() - lastImpact >= impactRepeat) {
    changeImage = true;
    lastImpact = millis();
    Serial.println(F("interrupt"));
  }
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  int  bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t  rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int  w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0;
  const uint8_t  gridWidth = ARRAY_WIDTH;
  const uint8_t  gridHeight = ARRAY_HEIGHT;

  //if((x >= gridWidth) || (y >= gridHeight)) {
    //Serial.print(F("Abort."));
    //return;
  //}

  Serial.println();

  if (!myFile.isOpen())
  {
    Serial.print(F("Loading image '"));
    Serial.print(filename);
    Serial.println('\'');
    // Open requested file on SD card
    if (!myFile.open(filename, O_READ)) {
      Serial.println(F("File open failed"));
      //sdErrorMessage();
      return;
    }
  }
  else myFile.rewind();

  // Parse BMP header
  if(read16(myFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(myFile));
    (void)read32(myFile); // Read & ignore creator bytes
    bmpImageoffset = read32(myFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(myFile));
    bmpWidth  = read32(myFile);
    bmpHeight = read32(myFile);
    if(read16(myFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(myFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(myFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        Serial.print(F("Image offset: "));
        Serial.print(offsetX);
        Serial.print(F(", "));
        Serial.println(offsetY);

        // image smaller than 16x16?
        if ((bmpWidth < ARRAY_WIDTH && bmpWidth > -ARRAY_WIDTH) || (bmpHeight < ARRAY_HEIGHT && bmpHeight > -ARRAY_HEIGHT))
        {
          clearStripBuffer();
        }

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;;
        Serial.print(F("Row size: "));
        Serial.println(rowSize);


        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= gridWidth)  w = gridWidth - x;
        if((y+h-1) >= gridHeight) h = gridHeight - y;

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).

          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = (bmpImageoffset + (offsetX * -3) + (bmpHeight - 1 - (row + offsetY)) * rowSize);
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(myFile.curPosition() != pos) { // Need seek?
            myFile.seekSet(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              myFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // push to LED buffer
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];

            // offsetY is beyond bmpHeight
            if (row >= bmpHeight - offsetY)
            {
              // black pixel
              leds[getIndex(col, row)] = CRGB::Black;
              //led[getIndex(col, row)] strip.Color(0, 0, 0));
            }
            // offsetY is negative
            else if (row < offsetY * -1)
            {
              // black pixel
              leds[getIndex(col, row)] = CRGB::Black;
              //strip.setPixelColor(getIndex(col, row), strip.Color(0, 0, 0));
            }
            // offserX is beyond bmpWidth
            else if (col >= bmpWidth + offsetX)
            {
              // black pixel
              leds[getIndex(col, row)] = CRGB::Black;
              //strip.setPixelColor(getIndex(col, row), strip.Color(0, 0, 0));
            }
            // offsetX is positive
            else if (col < offsetX)
            {
              // black pixel
              leds[getIndex(col, row)] = CRGB::Black;
              //strip.setPixelColor(getIndex(col, row), strip.Color(0, 0, 0));
            }
            // all good
            else {
              leds[getIndex(col+x, row)].setRGB(r, g, b);
              //strip.setPixelColor(getIndex(col+x, row), strip.Color(r, g, b));
            }
            // paint pixel color
          } // end pixel
        } // end scanline
      } // end goodBmp
    }
  }
  FastLED.show();

  if (singleGraphic == false || setupActive == true)
  {
    Serial.println(F("Closing Image..."));
    myFile.close();
  }
  if(!goodBmp) Serial.println(F("Format unrecognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(SdFile& f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(SdFile& f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void clearStripBuffer() {
  for (int i=0; i<NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
}

byte getIndex(byte x, byte y) {
  byte index;
  if (y % 2 == 0)
  {
    // even lines are the right way round
    // (0,2,4,6,8)
    index = y * 10 + x;
  } else {
    // odd number lines are reversed direction
    // (1,3,5,7,9)
    index = (y * 10 + 9) - x - 1;
  }
  return index;
}

bool getImageDimensions(char *filename) {

  const uint8_t  gridWidth = ARRAY_WIDTH;
  const uint8_t  gridHeight = ARRAY_HEIGHT;

  //if((0 >= gridWidth) || (0 >= gridHeight)) {
    //Serial.print(F("Abort."));
    //return false;
  //}

  // Open requested file on SD card
  if (!myFile.open(filename, O_READ)) {
    Serial.println(F("File open failed"));
    return false;
  }

  // Parse BMP header
  if(read16(myFile) == 0x4D42) { // BMP signature
    (void)read32(myFile); // Read & ignore file size
    (void)read32(myFile); // Read & ignore creator bytes
    (void)read32(myFile); // skip data
    // Read DIB header
    (void)read32(myFile); // Read & ignore Header size
    imageWidth  = read32(myFile);
    imageHeight = read32(myFile);
    Serial.print(F("Image resolution: "));
    Serial.print(imageWidth);
    Serial.print(F("x"));
    Serial.println(imageHeight);
  }
  Serial.println(F("Closing Image..."));
  myFile.close();

  if (imageWidth > ARRAY_WIDTH) {
    image_state = HORIZONTAL;
    Serial.println(F("Horizontal scrolling"));
    // for horizontal frames, move one led right per frame
    numFrames = imageWidth - ARRAY_WIDTH;
  } else if (imageHeight > ARRAY_HEIGHT) {
    Serial.println(F("vertically stacked animation frames"));
    image_state = VERTICAL;
    // for vertical animations, each frame is one display high
    numFrames = imageHeight / ARRAY_HEIGHT;
  } else {
    // single size image
    Serial.println(F("static, single image"));
    image_state = STATIC;
  }
  return true;
}

int showImage(char* filename, uint32_t frame_delay, uint8_t first_frame, uint16_t num_frames, uint16_t repeat_number) {
  int frameNum = 0;

  if (getImageDimensions(filename) == false) {
    // the image failed for some reason
    return frameNum;
  }

  if (image_state == STATIC) {
    offsetX = 0;
    offsetY = 0;
    singleGraphic = true;
    bmpDraw(filename, 0, 0);
    myFile.close();
    frameNum++;
    lastImageUpdate = millis();
    return frameNum;
  }

  Serial.print(F("first_frame: "));
  Serial.println(first_frame);

  // limit the number of displayed frames to the maximum available
  if (first_frame >= numFrames) {
    // loop back to the beginning
    first_frame = 0;
  }
  if (num_frames == 0) {
    //set to the maximum if given zero (play all)
    num_frames = numFrames - first_frame;
  }
  if (num_frames > numFrames - first_frame) {
    // clip to the maximum number
    num_frames = numFrames - first_frame;
  }

  Serial.print(F("num_frames: "));
  Serial.println(num_frames);

  for (int16_t i=0; i<repeat_number; i++) {
    currentFrame = first_frame;
    for (currentFrame; currentFrame<(first_frame+num_frames); currentFrame++) {
      if (image_state == HORIZONTAL) {
        offsetX = -currentFrame;
        offsetY = 0;
        singleGraphic = false;
        bmpDraw(filename, 0, 0);
        frameNum++;
      } else if (image_state == VERTICAL) {
        offsetX = 0;
        offsetY = currentFrame*ARRAY_HEIGHT;
        singleGraphic = false;
        bmpDraw(filename, 0, 0);
        frameNum++;
      }
      if (num_frames != 1) {
        // if we're just showing one frame, don't delay
        delay(frame_delay);
      }
    }
  }
  singleGraphic = true;
  //bmpDraw(filename, 0, 0);
  myFile.close();
  lastImageUpdate = millis();
  return frameNum;
}