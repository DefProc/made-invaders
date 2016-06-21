SOMO-II Arduino Library
=======================

An Arduino library to communicate with a SOMO-II module to play MP3 files.

Usage
-----

Set up the required Serial object based on the pin connections
you are using. Remember that the `TX` pin on the microcontroller board should
be connected to the `RX` pin on the SOMO-II board, and vice-versa.

```
  RX (Arduino) ----------------- TX (SOMO-II)
  TX (Arduino) ----------------- RX (SOMO-II)
```

When connecting to arbitrary pins on the Arduino, you'll need to use SoftwareSerial
to create a secondary serial connection.
In your Arduino Sketch, include and construct both the `SoftwareSerial` and `SOMO_II` items:

```
#include <SoftwareSerial.h>
#include <SOMO_II.h>

#define RX_PIN 8  // connect to SOMO-II TX
#define TX_PIN 8  // connect to SOMO-II RX

// set up a new serial object
SoftwareSerial mySerial = SoftwareSerial(RX_PIN, TX_PIN);

SOMO_II mp3 = SOMO_II(mySerial);
```

Or, if the SOMO-II is connected to a spare hardware serial port:

```
#include <SOMO_II.h>
SOMO_II mp3 = SOMO_II(Serial1);
```

Start both the serial port at the required baud rate, and then call `SOMO_II::begin()`
during the setup function:

```
mySerial.begin(SOMO_BAUD);
mp3.begin();
```

or

```
Serial1.begin(SOMO_BAUD);
mp3.begin();
```

This will start the SOMO-II module and set the volume to the mid-level.

Playing MP3 files
-----------------

In it's simplest form, you can call `playTrack(track_num)` to play tracks based on
the order they were copied onto the storage media, but if the tracks are arranged
into numbered folders (eg. `001`) and the files have preceding numbers (eg.
`001file.mp3`), then the `playFile(folder_num, file_num)` function is a little
more useful.

To change the volume, there are `volUp()` and `volDown()` functions to step up or
down the volume range (0–30) in increments of 1, or the `setVol(volume_level)` to
set it to a specific value.

The functions are intended to be used with `feedback()` on (`feedback(true)`),
currently the default, and they will then return a value based on if they are
successful (`SOMO_OK`) or not, and will return the error code.
