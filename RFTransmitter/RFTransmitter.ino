#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/* Gamma Correction Table */
const uint8_t PROGMEM gammaTable[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#define CE_PIN          9
#define CSN_PIN         10
#define BUTTON_ONE_PIN  2 
#define BUTTON_TWO_PIN  3

#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0

#define LED_MAX     255
#define LOOP_DELAY  1



const uint8_t address_book[20][6] = {
  "00001", "00002", "00003", "00004", "00005",
  "00006", "00007", "00008", "00009", "00010",
  "00011", "00012", "00013", "00014", "00015",
  "00016", "00017", "00018", "00019", "00020",
};

RF24 radio(CE_PIN, CSN_PIN);       



class Timer {
  private:
    uint16_t _count;           
    uint16_t _step;       
    uint16_t _max;      
    uint16_t _overflow;
    uint64_t _wait;
    uint64_t _last;   
    
  public:
    Timer(uint16_t count, uint16_t step, uint16_t max, uint16_t overflow, uint64_t wait);
    ~Timer();
    uint16_t tic(uint64_t curr_ms);
};



typedef enum {
  OFF = 0,
  IDLE = 1,
}strip_state_t;

class LedStrip {
  private:
    static uint8_t s_address_counter;

    uint8_t _address;
    strip_state_t _state;
    uint8_t _red;
    uint8_t _green;
    uint8_t _blue;
    //uint16_t _count;
    Timer *_timer;

    void idle();

  public:
  uint16_t _count;
    LedStrip(strip_state_t state);
    ~LedStrip();
    uint8_t getAddress();
    uint32_t getPacket();
    void setState(strip_state_t state);
    void update(uint64_t curr_ms);
};

uint8_t LedStrip::s_address_counter = 0;



void sendMessage(uint8_t address, uint32_t message);

uint64_t g_curr_ms = 0;
LedStrip* strips[20];
int g_num_strips = 0;

boolean g_button_one_state;


void setup() 
{
  pinMode(BUTTON_ONE_PIN, INPUT);

  strips[g_num_strips++] = new LedStrip(OFF);

  Serial.begin(9600);
 
  radio.begin();                  //Starting the Wireless communication
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();

  strips[0]->setState(IDLE);
}

void loop()
{  
  g_curr_ms = millis();

  // = digitalRead(BUTTON_ONE_PIN);
  

  for(int i = 0; i < g_num_strips; i++)
  {
    strips[i]->update(g_curr_ms);
  }
  
  sendMessage(strips[0]->getAddress(), strips[0]->getPacket());
  delay(LOOP_DELAY);
}



Timer::Timer(uint16_t count, uint16_t step, uint16_t max, uint16_t overflow, uint64_t wait)
{
  _count = count;
  _step = step;
  _max = max;
  _overflow = overflow;
  _wait = wait;
  _last = millis();
}

Timer::~Timer()
{
  return;
}
    
uint16_t Timer::tic(uint64_t curr_ms)
{
  if(_step != 0 && curr_ms - _last >= _wait)
  {
    _last = curr_ms;
    _count += _step;
    if(_count >= _max)
    {
      _count = _overflow;
    }
  }

  return _count;
}



LedStrip::LedStrip(strip_state_t state = OFF)
{
  _address = s_address_counter;
  s_address_counter++;
  _state = state;
  _red = 0;
  _green = 0;
  _blue = 0;
  _count;
  _timer = NULL;

  setState(_state);
}
    
LedStrip::~LedStrip()
{
  delete _timer;
}

void LedStrip::idle()
{
  Wheel(_count, &_red, &_green, &_blue);
}

uint8_t LedStrip::getAddress()
{
  return _address;
}

uint32_t LedStrip::getPacket()
{
  uint32_t packet = 0;

  _red = pgm_read_byte(&gammaTable[_red]);
  _green = pgm_read_byte(&gammaTable[_green]);
  _blue = pgm_read_byte(&gammaTable[_blue]);

  packet |= (uint32_t)_red << RED_SHIFT;
  packet |= (uint32_t)_green << GREEN_SHIFT;
  packet |= (uint32_t)_blue << BLUE_SHIFT;

  return packet;
}

void LedStrip::setState(strip_state_t state)
{
  switch (state)
  {
    case OFF:
      _state = state;

      _red = 0;
      _green = 0;
      _blue = 0;

      delete _timer;
      _timer = new Timer(0,0,0,0,0);
      break;

    case IDLE:
      _state = state;

      delete _timer;
      _timer = new Timer(0,1,256,0,50);
      break;

    default:
      break;
  }
}

void LedStrip::update(uint64_t curr_ms)
{
  switch (_state)
  {
    case OFF:
      break;

    case IDLE:
      idle();
      break;

    default:
      break;
  }

  _count = _timer->tic(curr_ms);
}



void sendMessage(uint8_t address, uint32_t message)
{
  radio.openWritingPipe(address_book[address]);
  radio.write(&message, sizeof(message)); 
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void Wheel(uint8_t p, uint8_t *r, uint8_t *g, uint8_t *b) 
{
  p = 255 - p;
  
  if(p < 85) 
  {
    *r = LED_MAX - p * 3;
    *g = 0;
    *b = p * 3;
  }
  else if(p < 170) 
  {
    p -= 85;
    *r = 0;
    *g = p * 3;
    *b = LED_MAX - p * 3;
  }
  else
  {
    p -= 170;
    *r = p * 3;
    *g = LED_MAX - p * 3;
    *b = 0;
  }
}
