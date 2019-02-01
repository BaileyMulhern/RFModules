#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN          9
#define CSN_PIN         10
#define BUTTON_ONE_PIN  2 
#define BUTTON_TWO_PIN  3

#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0

#define LED_MAX 255

RF24 radio(CE_PIN, CSN_PIN);       
const byte address[6] = "00001";     //Byte of array representing the address. This is the address where we will send the data. This should be same on the receiving side.
boolean button_one_state = 0;
boolean button_two_state = 0;

uint32_t message;
void encode(uint32_t *message);
uint8_t g_red;
uint8_t g_green;
uint8_t g_blue;

int count;
int count_step;
int count_max;
int wait;



class Timer {
  private:
    uint64_t wait;
    uint64_t last_tic;
    uint16_t count;           //Counter variable to keep track of where in the effect the task is
    uint16_t count_max;       //Max number of counts before the counter rolls over
    uint16_t count_step;      //Amount the counter increases each time its incremented
    uint16_t count_overflow;  //Value that the counter overflows to 
    
  public:
    Timer(uint64_t wait, uint16_t count, uint16_t count_max, uint16_t count_step, uint16_t count_overflow);
    uint16_t tic(uint64_t curr_ms);
};

typedef enum {
  OFF = 0,
  IDLE = 1,
  TEST = 2,
}strip_state_t;

class LedStrip {
  private:
    uint8_t address[6];
    uint32_t packet;
    strip_state_t state;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t count;
    Timer *counter;

    void off();
    void idle();
    void test();
  public:
    LedStrip(const uint8_t *address[6]);
    ~LedStrip();
    uint32_t getPacket();
    void setState(strip_state_t state);
    void update();
};

void setup() 
{
  //pinMode(BUTTON_ONE_PIN, INPUT);
  //pinMode(BUTTON_TWO_PIN, INPUT);
  
  g_red = 0;
  g_green = 0;
  g_blue = 0;

  count = 0;
  count_step = 1;
  count_max = 256;
  wait = 50;

  Serial.begin(9600);
 
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();          //This sets the module as transmitter
}

void loop()
{
  message = 0;
  
  //button_one_state = digitalRead(BUTTON_ONE_PIN);
  //button_two_state = digitalRead(BUTTON_TWO_PIN);

  //message |= (uint32_t) button_one_state * 0xFF << RED_SHIFT;
  //message |= (uint32_t) button_two_state * 0xFF << BLUE_SHIFT;

  Wheel(count);

  encode(&message);
  Serial.println(message, HEX);

  Serial.print(g_red, HEX);
  Serial.print("\t");
  Serial.print(g_green, HEX);
  Serial.print("\t");
  Serial.print(g_blue, HEX);
  Serial.println();

  radio.write(&message, sizeof(message));  //Sending the message to receiver 

  count += count_step;
  if(count >= count_max)
  {
    count = 0;
  }
  delay(wait);
}

Timer::Timer(uint64_t wait, uint16_t count, uint16_t count_max, uint16_t count_step, uint16_t count_overflow)
{
  this->count = count;
  this->count_max = count_max;
  this->count_step = count_step;
  this->count_overflow = count_overflow;
  this->wait = millis();
}
    
uint16_t Timer::tic(uint64_t curr_ms)
{
  if(this->count_step != 0 && curr_ms - this->last_tic >= this->wait)
  {
    this->last_tic = curr_ms;
    this->count += this->count_step;
    if(this->count >= this->count_max)
    {
      this->count = this->count_overflow;
    }
  }

  return this->count;
}

LedStrip::LedStrip(const uint8_t *address[6])
{
  this->address = *address;
  this->packet = 0;
  this->red;
  this->green;
  this->blue;
  this->count;
  this->counter = new Timer(0,0,0,0,0);

  setState(strip_state_t::OFF);
}
    
LedStrip::~LedStrip
{
  if(counter != NULL)
  {
    delete counter;
  }
}

uint32_t LedStrip::getPacket();

void LedStrip::update();

void encode(uint32_t *message)
{
  uint32_t temp = 0;
  temp |= (uint32_t)g_red << RED_SHIFT;
  temp |= (uint32_t)g_green << GREEN_SHIFT;
  temp |= (uint32_t)g_blue << BLUE_SHIFT;
  *message = temp;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void Wheel(uint8_t p) 
{
  p = 255 - p;
  
  if(p < 85) 
  {
    g_red = LED_MAX - p * 3;
    g_green = 0;
    g_blue = p * 3;
  }
  else if(p < 170) 
  {
    p -= 85;
    g_red = 0;
    g_green = p * 3;
    g_blue = LED_MAX - p * 3;
  }
  else
  {
    p -= 170;
    g_red = p * 3;
    g_green = LED_MAX - p * 3;
    g_blue = 0;
  }
}
