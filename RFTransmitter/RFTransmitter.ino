#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN         
const byte address[6] = "00001";     //Byte of array representing the address. This is the address where we will send the data. This should be same on the receiving side.
uint8_t button_one = 2;
uint8_t button_two = 3;
boolean button_one_state = 0;
boolean button_two_state = 0;

typedef struct message_t 
{
  uint8_t RED_LED:1;
  uint8_t BLUE_LED:1;
}Message;

void setup() 
{
  pinMode(button_one, INPUT);
  pinMode(button_two, INPUT);
 
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MIN);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();          //This sets the module as transmitter
}
void loop()
{
  message = 0;
  
  button_one_state = digitalRead(button_one);
  button_two_state = digitalRead(button_two);

  message |= button_one_state << Message.RED_LED;
  message |= button_two_state << Message.BLUE_LED;
  
  radio.write(&message, sizeof(message));  //Sending the message to receiver 
  delay(10);
}
