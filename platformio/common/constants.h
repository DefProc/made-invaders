// some basic constants for everybody
#define BAUD 115200 // serial baud rate is then contsant accross all devices
#define RFID_DIGITS 16 // max number of chars to hold an scanned ID
#define NUM_TARGETS 16 // how many targets do we have

// some unsigned long values for the various timer lengths
#define RUN_TIMER 30000UL // play time in ms
#define GRACE_TIMER 250UL // this is a bit of a fude so it *looks* like the game ends at zero
#define EARLY_PLAY 1000UL
#define EXTRA_TIMER 10000UL // how much extra time for a zero score
#define IDLE_TIMER 60000UL // wait a minute before going into attract mode
#define COUNT_DOWN 5000UL

// RFM69 constants
#define NETWORKID     101  // The same on all nodes that talk to each other
#define MAIN_CTRL     200  // The Main Controller
#define REG_STN       201  // Registration station interpretter (to RPi)
#define TIMER         202  // Countdown Timer
#define SCOREBD       203  // The Score Board
#define FREQUENCY     RF69_868MHZ
#define IS_RFM69HW

// set the possible messages (in message_id)
enum message_t : uint8_t {
  GAME_START,
  TAKE_PHOTO,
  GAME_END,
  NOTHING_DOING,
  CANCEL_GAME,
  GET_READY,
  HIT,
  DISPLAY_NUM,
  MORE_TIME,
  FALSE_START
};

typedef struct {
  message_t message_id = GAME_START;
  uint32_t impact_num = 0UL;
  long timestamp = 0L;
  char rfid_num[RFID_DIGITS+1];
  uint32_t game_uid;
  long score = 0L;
} Payload;

enum game_states_t {
  IDLE,
  RFID_SCANNED,
  COUNTDOWN,
  RUNNING,
  EXTRA_TIME,
  END_GAME
};

// to allow builtin LED use on Moteino/Moteino Mega
#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif
