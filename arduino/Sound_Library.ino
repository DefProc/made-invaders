
// This is the sound Library for Made Invaders
//by Ed Saul, June 2016

  // sets up a serial port on digital pins 40 and 38 as Made Invaders ised the Mega Serial Ports and so it can be used on Arduinos with less functionality
  // Improtant to use a step down resistor (1KOhm) on Pin 38 as Arduino has 5V output and SOMO needs 3.2
#include <SoftwareSerial.h>
// SOMO Serial pins
int Rx_pin = 40;
int Tx_pin = 38;
// create the object (Sound Card) for the virtual serial port. 
SoftwareSerial Card(Rx_pin,Tx_pin);

// create the object for the serial out on Arduino Mega
//#define Card Serial
// define a structure for holding the bytes that will be passed to the SOMO
typedef struct _ControlMsg
{
 byte start;
 byte cmd;
 byte feedback;
 byte para1;
 byte para2;
 byte checksum1;
 byte checksum2;
 byte end;
} ControlMsg;

byte start = 0x7E;// always start with 7E
byte feedback = 0; // sets the feedback requirement to off

// This initializes the SOMO
void setup(void)
{
 Initialize_SOMO(); //see initialise command below which sets volume
}
// Command calls in loop
void loop(void)
{
// Insert Play Track and/or volume commands as appropriate
Vol_Set(30); //This sets the volume , Max 30
Play_Track(1,7); // This plays folder x, track y on the Card. See Play_Track command below

}
// here is the function for initializing the SOMO
void Initialize_SOMO()
{

 // initialize the serial port to the SOMO MP3 Sound Card module
 Card.begin(9600);

}

// Soundcard Library: 
// This sets the volume
void Vol_Set(byte vol)
{
 ControlMsg ctrlMsg; // Create structure

 // set variables for setting volume
 byte cmd = 0x06; // invokes the set volume command
 byte para1 = 0; // always 0 in this command
 byte para2 = vol; // sets the volume between 0 and 30)
 // populate structure with the bytes
  Common_Msg(cmd, para1, para2);
 
}

// This plays tracks
void Play_Track(byte folder, byte track)
{
 ControlMsg ctrlMsg; // Create structure

 // set variables for playing a track

 byte cmd = 0x0F; // Invokes play specified folder & track command
 byte para1 = folder; // sets it to folder "01" on the microSD card
 byte para2 = track; // sets the track number (1 to 255)
 // populate structure with the bytes
 Common_Msg(cmd, para1, para2);
 
}

void Common_Msg(byte cmd, byte para1, byte para2)
{
 ControlMsg ctrlMsg;
 ctrlMsg.start = start; // always 7E
 ctrlMsg.cmd = cmd;
 ctrlMsg.feedback = feedback;
 ctrlMsg.para1 = para1;
 ctrlMsg.para2 = para2;
 // Calculating Checksum (2 bytes) = 0xFFFF – (CMD + Feedback + Para1 + Para2) + 1
 word chksum = 0xFFFF - ((word)cmd + (word)feedback + (word)para1 +
(word)para2) + 1;
 ctrlMsg.checksum1 = (byte)(chksum >> 8); // upper 8 bits
 ctrlMsg.checksum2 = (byte)chksum; // lower 8 bits
 ctrlMsg.end = 0xEF; // always use this end byte
 // Now we write the structure to the SOMO MP3 Module
 Card.write((const byte*)&ctrlMsg, sizeof(ctrlMsg));
 Card.flush();
 }

