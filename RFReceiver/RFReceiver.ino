#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Adafruit_TLC59711.h"

#define CE_PIN        9
#define CSN_PIN       10
#define MOSI_PIN      11
#define CLOCK_PIN     13
#define RED_LED_PIN   2 
#define BLUE_LED_PIN  3

#define RED_LED   1
#define BLUE_LED  1 << 1

#define NUM_TLC59711 1

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

uint8_t message;

Adafruit_TLC59711 tlc = Adafruit_TLC59711(NUM_TLC59711, CLOCK_PIN, MOSI_PIN);

void setup() 
{
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);   //Setting the address at which we will receive the data
  radio.setPALevel(RF24_PA_MIN);       //You can set this as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.startListening();              //This sets the module as receiver

  tlc.begin();
  tlc.write();
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

    if(message & RED_LED)
    {
      digitalWrite(RED_LED_PIN, HIGH);
      tlc.setLED(0, 0, 65535, 65535);
    }
    else
    {
      digitalWrite(RED_LED_PIN, LOW);
    }
    
    if(message & BLUE_LED)
    {
      digitalWrite(BLUE_LED_PIN, HIGH);
      tlc.setLED(0, 65535, 65535, 0);
    }
    else
    {
      digitalWrite(BLUE_LED_PIN, LOW);
    }

    tlc.write();
  }
  delay(5);
} 
