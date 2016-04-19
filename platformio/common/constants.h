typedef struct {
  uint8_t message_id = 0x00;
  uint32_t impact_num = 0UL;
  int32_t timestamp = 0x0000;
  uint8_t rfid_num[8];
  uint32_t game_uid;
  uint32_t score = 0UL;
} Payload;

#define GAME_START 0x00
#define TAKE_PHOTO 0x01
#define GAME_END   0x02
#define CANCEL_GAME 0x03
#define GET_READY  0x04
#define HIT        0x05

// RFM69 constants
#define NETWORKID     101  // The same on all nodes that talk to each other
#define MAIN_CTRL     200  // The Main Controller
#define REG_STN       201  // Registration station interpretter (to RPi)
#define TIMER         202  // Countdown Timer
#define SCOREBD       203  // The Score Board
#define FREQUENCY     RF69_915MHZ
#define IS_RFM69HW
