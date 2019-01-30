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

RF24 radio(CE_PIN, CSN_PIN);       
const byte address[6] = "00001";     //Byte of array representing the address. This is the address where we will send the data. This should be same on the receiving side.
boolean button_one_state = 0;
boolean button_two_state = 0;

uint8_t message;

void setup() 
{
  pinMode(BUTTON_ONE_PIN, INPUT);
  pinMode(BUTTON_TWO_PIN, INPUT);

  Serial.begin(9600);
 
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();          //This sets the module as transmitter
}
void loop()
{
  message = 0;
  
  button_one_state = digitalRead(BUTTON_ONE_PIN);
  button_two_state = digitalRead(BUTTON_TWO_PIN);

  message |= (button_one_state * 0xFF) << RED_SHIFT;
  message |= (button_two_state * 0xFF) << BLUE_SHIFT;

  radio.write(&message, sizeof(message));  //Sending the message to receiver 
  delay(5);
}
