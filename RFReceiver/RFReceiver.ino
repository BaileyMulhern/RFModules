#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN          9
#define CSN_PIN         10
#define MOSI_PIN        11
#define CLOCK_PIN       13
#define RED_LED_PIN     2  
#define BLUE_LED_PIN    4
#define STRIP_RED_PIN   3
#define STRIP_GREEN_PIN 5
#define STRIP_BLUE_PIN  6

#define RED_LED   1
#define BLUE_LED  2

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
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

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
    delay(5);
  }

  analogWrite(STRIP_RED_PIN, red);
  analogWrite(STRIP_GREEN_PIN, green);
  analogWrite(STRIP_BLUE_PIN, blue);

  delay(5);
} 

void decode(uint32_t message)
{
  red = (uint8_t) (message >> RED_SHIFT);
  green = (uint8_t) (message >> GREEN_SHIFT);
  blue = (uint8_t) (message >> BLUE_SHIFT);
}
