#include <FastLED.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN      9
#define CSN_PIN     10
#define BUTTON_PIN  2 

#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0

#define LED_MAX         255
#define BYTE_MAX        256
#define LOOP_DELAY      1
#define DEBOUNCE_DELAY  50

#define SIN_OFFSET  193
#define COS_OFFSET  129

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

const uint8_t address_book[20][6] = {
  "00001", "00002", "00003", "00004", "00005",
  "00006", "00007", "00008", "00009", "00010",
  "00011", "00012", "00013", "00014", "00015",
  "00016", "00017", "00018", "00019", "00020",
};      


/*****************************************************************
 * Timer Class
 * 
 * Responsible for incrementing the counter after the given wait
 */
class Timer {
  private:
    uint16_t _count;    //Counter value
    uint16_t _step;     //Value that the counter is incremented by each period
    uint16_t _max;      //Upper bound of the counter
    uint16_t _overflow; //Value that the counter is set to once it overflows
    uint64_t _wait;     //Wait time in milliseconds between counter increments
    uint64_t _last;     //Time in milliseconds since the timer last ticked
    
  public:
    uint64_t _num_overflow; //Number of times the counter has overflowed

    Timer(uint16_t count, uint16_t step, uint16_t max, uint16_t overflow, uint64_t wait);
    ~Timer();
    uint16_t tic(uint64_t curr_ms);
};


/*****************************************************************
 * Strip State Enum
 * 
 * Enum for each effect possible for the LED strips
 */
typedef enum {
  OFF = 0,
  RAINBOW = 1,
	RGB_SINE = 2,
  RGB_TRI = 3,
  RGB_QUAD = 4,
  RGB_CUBIC = 5,
  NUM_STATES = 6,
}strip_state_t;

/*****************************************************************
 * LedStrip Class
 * 
 * Class for each led strip to be transmitted to by the radio
 */
class LedStrip {
  private:
    static uint8_t s_address_counter; //The current total number of RF addresses

    uint8_t _address;                 //The RF address of the strip
    strip_state_t _state;             //The current effect state of the strip
    CRGB _color;                      //The color of the strip
    uint16_t _count;                  //Current counter value of the timer
    Timer *_timer;                    //The strips internal timer

    void rainbow();
    void rgbTri();
		void rgbQuad();
		void rgbCubic();
		void rgbSine();

  public:
    LedStrip(strip_state_t state);
    ~LedStrip();
    uint8_t getAddress();
    uint32_t getPacket();
    void setState(strip_state_t state);
    void update(uint64_t curr_ms);
};

//Initialize the number of addresses to 0
uint8_t LedStrip::s_address_counter = 0;

//Helper Functions
void sendMessage(uint8_t address, uint32_t message);
void readButton();

//Initialize the RF module
RF24 radio(CE_PIN, CSN_PIN); 

//Initialize global variables
uint64_t g_curr_ms = 0; //current milliseconds

LedStrip* strips[20];       //Array of all of the LED strips
uint8_t g_num_strips = 0;   //Number of LED strips
uint8_t g_curr_effect = 0;  //Integer value of the current effect 

boolean g_curr_button_state = 0;  //Current button state
boolean g_last_button_state = 0;  //Previous button state
boolean g_button_unpressed = 0;   //Flag for when the button has been unpressed
uint64_t g_last_button_debounce;  //Milliseconds since the button was last debounced



void setup() 
{
  //Setup the button
  pinMode(BUTTON_PIN, INPUT);

  //Initialize all of the LED strips
  strips[g_num_strips++] = new LedStrip(OFF);
  strips[0]->setState(g_curr_effect);
 
  //Setup the RF transmitter
  radio.begin();                  //Starting the Wireless communication
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();

  //Start serial line
  Serial.begin(9600);
}

void loop()
{ 
  //Update the current time 
  g_curr_ms = millis();

  //Read from the button
  readButton();
  //If the button is unpressed, increment through the effects
  if(g_button_unpressed)
  {
    //Reset the unpressed flag
    g_button_unpressed = 0;
    //Increment the effect
    g_curr_effect++;
    g_curr_effect = g_curr_effect % NUM_STATES;
    //Update the current state of the strip
    strips[0]->setState(g_curr_effect);
  }
  
  //Iterate through all of the strips and update them all
  for(int i = 0; i < g_num_strips; i++)
  {
    strips[i]->update(g_curr_ms);
  }
  
  //Send the color out to the receiver
  sendMessage(strips[0]->getAddress(), strips[0]->getPacket());


  delay(LOOP_DELAY);
}



/*****************************************************************
 * Timer Methods
 */

/*
 * Timer Constructor
 */
Timer::Timer(uint16_t count, uint16_t step, uint16_t max, uint16_t overflow, uint64_t wait)
{
  _count = count;
  _step = step;
  _max = max;
  _overflow = overflow;
  _num_overflow = 0;
  _wait = wait;
  _last = millis();
}

/*
 * Timer Deconstructor
 */
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
      _num_overflow++;
    }
  }

  return _count;
}



/*****************************************************************
 * LedStrip Methods
 */

/*
 * LedStrip Constructor
 */
LedStrip::LedStrip(strip_state_t state = OFF)
{
  _address = s_address_counter++;
  _state = state;
  _color = CRGB::Black;
  _timer = NULL;

  setState(_state);
}

/* 
 * LedStrip()
 *
 * LedStrip Deconstructor
 */
LedStrip::~LedStrip()
{
  delete _timer;
}

/*
 * rainbow()
 * 
 * Cycles through the rainbow based on the value of _count
 * 
 * 	_count		0
 *  _step		  1
 *  _max		  256
 * 	_overflow 0
 * 	_wait		  50
 */
void LedStrip::rainbow()
{
  _color = CHSV((uint8_t)_count, LED_MAX, LED_MAX);
}

/*
 * rgbTri()
 * 
 * Pulses through a triangle wave in red, green, and blue
 * 
 * 	_count		0
 *  _step		  3
 *  _max		  768
 * 	_overflow	0
 * 	_wait		  10
 */
void LedStrip::rgbTri()
{
	_color = CRGB::Black;

  if(_count < BYTE_MAX)
  {
		_color.r = triwave8(_count);
  }
	else if(_count < 2 * BYTE_MAX)
	{
		_color.g = triwave8(_count % BYTE_MAX);
	}
	else
	{
		_color.b = triwave8(_count % BYTE_MAX);
	}
}

/*
 * rgbQuad()
 * 
 * Pulses through a quadratic wave in red, green, and blue
 * 
 * 	_count		0
 *  _step		  3
 *  _max		  768
 * 	_overflow	0
 * 	_wait		  10
 */
void LedStrip::rgbQuad()
{
	_color = CRGB::Black;

  if(_count < BYTE_MAX)
  {
		_color.r = quadwave8(_count);
  }
	else if(_count < 2 * BYTE_MAX)
	{
		_color.g = quadwave8(_count % BYTE_MAX);
	}
	else
	{
		_color.b = quadwave8(_count % BYTE_MAX);
	}
}

/*
 * rgbCubic()
 * 
 * Pulses through a cubic wave in red, green, and blue
 * 
 * 	_count		0
 *  _step		  3
 *  _max		  768
 * 	_overflow	0
 * 	_wait		  10
 */
void LedStrip::rgbCubic()
{
	_color = CRGB::Black;

  if(_count < BYTE_MAX)
  {
		_color.r = cubicwave8(_count);
  }
	else if(_count < 2 * BYTE_MAX)
	{
		_color.g = cubicwave8(_count % BYTE_MAX);
	}
	else
	{
		_color.b = cubicwave8(_count % BYTE_MAX);
	}
}

/*
 * rgbSine()
 * 
 * Pulses through a sine wave in red, green, and blue
 * 
 * 	_count		0
 *  _step		  3
 *  _max		  768
 * 	_overflow	0
 * 	_wait		  10
 */
void LedStrip::rgbSine()
{
	_color = CRGB::Black;

  _color.r = _count;
  _color.g = LED_MAX - _count;

  int frac = (int) BYTE_MAX / 8;
  if(_count >= 7 * frac)
  {
    int num_pulses = 4;

    int count = _count - 7 * frac;
    count = count % (frac / num_pulses);
    count = count * 128 / (frac / num_pulses); 
    _color.r = sin8(count); 
  }
}

/* 
 * getAddress()
 *
 * Returns the address of the strip 
 */
uint8_t LedStrip::getAddress()
{
  return _address;
}

/* 
 * getPacket()
 *
 * Returns the 32 bit RGB color of the strip
 */
uint32_t LedStrip::getPacket()
{
  uint32_t packet = 0;

  packet |= (uint32_t)_color.r << RED_SHIFT;
  packet |= (uint32_t)_color.g << GREEN_SHIFT;
  packet |= (uint32_t)_color.b << BLUE_SHIFT;

  return packet;
}

/* 
 * setState()
 *
 * Transitions the strip to the given state, setting the
 * initial conditions for the state 
 */
void LedStrip::setState(strip_state_t state)
{
  switch (state)
  {
    case OFF:
      _state = state;

      _color = CRGB::Black;

      delete _timer;
      _timer = new Timer(0,0,0,0,0);
      break;

    case RAINBOW:
      _state = state;

			_color = CRGB::Red;

      delete _timer;
      _timer = new Timer(0,1,BYTE_MAX,0,50);
      break;

		case RGB_TRI:
      _state = state;

			_color = CRGB::Black;

      delete _timer;
      _timer = new Timer(0,3,3 * BYTE_MAX,0,10);
      break;

		case RGB_QUAD:
      _state = state;

			_color = CRGB::Black;

      delete _timer;
      _timer = new Timer(0,3,3 * BYTE_MAX,0,10);
      break;

		case RGB_CUBIC:
      _state = state;

			_color = CRGB::Black;

      delete _timer;
      _timer = new Timer(0,3,3 * BYTE_MAX,0,10);
      break;

		case RGB_SINE:
      _state = state;

			_color = CRGB::Black;

      delete _timer;
      uint64_t twoMinutes = 120.0 * 1000 / 256;
      _timer = new Timer(0,1,BYTE_MAX,0,twoMinutes);
      break;

    default:
      break;
  }
}

/* 
 * update()
 *
 * Updates the strip based on the current time and state
 */
void LedStrip::update(uint64_t curr_ms)
{
	//Update the current counter value
	_count = _timer->tic(curr_ms);

	//Run the effect method corresponding to the current state
  switch (_state)
  {
    case OFF:
      break;

    case RAINBOW:
      rainbow();
      break;

		case RGB_TRI:
      rgbTri();
      break;

		case RGB_QUAD:
      rgbQuad();
      break;

		case RGB_CUBIC:
      rgbCubic();
      break;

		case RGB_SINE:
      rgbSine();
      break;

    default:
      break;
  }
}



/* 
 * sendMessage()
 *
 * Sends the given package to the given address
 */
void sendMessage(uint8_t address, uint32_t message)
{
  radio.openWritingPipe(address_book[address]);
  radio.write(&message, sizeof(message)); 
}

/* 
 * readButton()
 *
 * Reads the state of the button with debounce, tripping 
 * the unpressed flag if the button goes from pressed to
 * unpressed
 */
void readButton()
{
  boolean reading = digitalRead(BUTTON_PIN);

  if (reading != g_last_button_state) 
  {
    g_last_button_debounce = millis();
  }

  if ((millis() - g_last_button_debounce) > DEBOUNCE_DELAY) 
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != g_curr_button_state) 
    {
      g_curr_button_state = reading;

      //If the button was pressed and now is unpressed, then set the g_button_unpressed flag
      g_button_unpressed = !g_curr_button_state;
    }
  }

  g_last_button_state = reading;
}
