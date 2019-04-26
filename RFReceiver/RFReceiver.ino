#include <EEPROM.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define LOOP_DELAY  10

//Pin Definitions
#define CE_PIN          9
#define CSN_PIN         10
#define MOSI_PIN        11
#define CLOCK_PIN       13
#define RED_PIN         3
#define GREEN_PIN       5
#define BLUE_PIN        6

//Defining Message Bits
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0

/* Serial Pipes */
#define WRITING_PIPE    0
#define READING_PIPE    1

enum EepromAddress {
    ADDRESS             = 0,
    CURRENT_EFFECT      = 1,
};

typedef enum : uint32_t {
    CONNECT             = 0,
    SET_ADDRESS         = 1,
    OFF                 = 2,
    DEPENDENT_MODE      = 3,
    INDEPENDENT_MODE    = 4,
    RESET               = 5,
}command_t;

RF24 radio(CE_PIN, CSN_PIN);
void initRF();
void readRF();
uint8_t node_index = 1;
const uint8_t master_address[6] = "0Node";

void updateEffect();
bool independent = true;
int current_effect = 0;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

void setup() 
{
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    current_effect = EEPROM.read(CURRENT_EFFECT);

    initRF();

    Serial.begin(9600);
}
void loop()
{
    readRF();

    if(independent)
    {
        /* Update red, green, blue based on the current effect */
    }

    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);

    delayMicroseconds(LOOP_DELAY);
} 

void initRF()
{
    /* Start the radio */
    radio.begin();

    /* Get node index from eeprom */
    node_index = EEPROM.read(ADDRESS);
    /* Ensure the index is 1 - 5 */
    if(0 > node_index || node_index > 5)
    {
        node_index = 1;
    }
    /* Set the address of the radio */
    uint8_t address[6] = "#Node";
    address[0] = node_index + '0';
    radio.openReadingPipe(READING_PIPE, address);  

    /* Open the writing pipe on 0 to master */
    radio.openWritingPipe(master_address);

    /* Set the distance from transmitter */
    radio.setPALevel(RF24_PA_MIN); 
    
    /* Start listening */
    radio.startListening();
}

void readRF()
{
    if(radio.available())
    {
        command_t command;
        radio.read(&command, sizeof(command_t));    
        delayMicroseconds(10);
    }
}

void updateEffect()
{
    
}

/*void decode(uint32_t message)
{
  red = (uint8_t) (message >> RED_SHIFT);
  green = (uint8_t) (message >> GREEN_SHIFT);
  blue = (uint8_t) (message >> BLUE_SHIFT);
}*/
