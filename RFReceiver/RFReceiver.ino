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

//Pin Definitions
#define CE_PIN          9
#define CSN_PIN         10
#define MOSI_PIN        11
#define CLOCK_PIN       13
#define STRIP_RED_PIN   3
#define STRIP_GREEN_PIN 5
#define STRIP_BLUE_PIN  6

//Defining Message Bits
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

uint32_t message;
void decode(uint32_t message);
uint8_t red;
uint8_t green;
uint8_t blue;

void setup() 
{
  pinMode(STRIP_RED_PIN, OUTPUT);
  pinMode(STRIP_GREEN_PIN, OUTPUT);
  pinMode(STRIP_BLUE_PIN, OUTPUT);

  red = 0;
  green = 0;
  blue = 0;

  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);   //Setting the address at which we will receive the data
  radio.setPALevel(RF24_PA_MIN);       //You can set this as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.startListening();              //This sets the module as receiver
}
void loop()
{
  if (radio.available())              //Looking for the data.
  {
    message = 0;
    //char text[32] = "";                 //Saving the incoming data
    //radio.read(&text, sizeof(text));    //Reading the data
    radio.read(&message, sizeof(message));    //Reading the data
    //You need this delay dont ask me why
    delayMicroseconds(10);

    decode(message);
  }

  analogWrite(STRIP_RED_PIN, red);
  analogWrite(STRIP_GREEN_PIN, green);
  analogWrite(STRIP_BLUE_PIN, blue);

  delayMicroseconds(10);
} 

void decode(uint32_t message)
{
  red = (uint8_t) (message >> RED_SHIFT);
  green = (uint8_t) (message >> GREEN_SHIFT);
  blue = (uint8_t) (message >> BLUE_SHIFT);
}
