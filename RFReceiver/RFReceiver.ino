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

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

uint8_t message;

void setup() 
{
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  pinMode(STRIP_RED_PIN, OUTPUT);
  pinMode(STRIP_GREEN_PIN, OUTPUT);
  pinMode(STRIP_BLUE_PIN, OUTPUT);

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

    if(message & RED_LED)
    {
      digitalWrite(RED_LED_PIN, HIGH);
      analogWrite(STRIP_RED_PIN, 255);
    }
    else
    {
      digitalWrite(RED_LED_PIN, LOW);
      analogWrite(STRIP_RED_PIN, 0);
    }
    
    if(message & BLUE_LED)
    {
      digitalWrite(BLUE_LED_PIN, HIGH);
      analogWrite(STRIP_BLUE_PIN, 255);
    }
    else
    {
      digitalWrite(BLUE_LED_PIN, LOW);
      analogWrite(STRIP_BLUE_PIN, 0);
    }
  }
  delay(5);
} 
