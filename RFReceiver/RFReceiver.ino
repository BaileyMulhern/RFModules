/* RF Receiver Node
 *
 * Wireless multi-node module that receives RF transmissions
 * and updates an LED strips based upon the commands sent by
 * the single RF transmitter module
 * 
 * Author: Bailey Mulhern
 */

#include <EEPROM.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <printf.h>

#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>

#include <FastLED.h>


/* Arduino Init Parameters */
#define SERIAL_BAUD     115200
/* Pin Definitions */
#define CE_PIN          9
#define CSN_PIN         10
#define MOSI_PIN        11
#define CLOCK_PIN       13
#define RED_PIN         5
#define GREEN_PIN       6
#define BLUE_PIN        3
/* Constants */
#define ANALOG_PIN_MAX  255
#define LOOP_DELAY      1


/* Shift for 32bit Colors */
#define RED_SHIFT       16
#define GREEN_SHIFT     8
#define BLUE_SHIFT      0

/* Radio Constants */
#define MIN_NODE_INDEX  2
#define MAX_NODE_INDEX  5
#define BYTES_PER_ADDRESS 6
/* Serial Pipes */
#define WRITING_PIPE    0
#define READING_PIPE    1
/* Ack Settings */
#define MAX_RETRIES     15

/* Color Macros */
#define OFF             0
#define RED             0x00FF0000U
#define GREEN           0x0000FF00U
#define BLUE            0x000000FFU
#define ORANGE          0x00FF7F00U
#define PURPLE          0x00FF00FFU

/* Effect Macros */
#define ONE_COLOR_COUNT_MAX     255
#define TWO_COLOR_COUNT_MAX     511
#define THREE_COLOR_COUNT_MAX   767
#define FOUR_COLOR_COUNT_MAX    1023
#define SINE_MAX                255
#define HALLOWEEN_MAX           511

/**************************************************
 *              Global Enumerations 
 **************************************************/

//Eeprom indexes for each variable 
typedef enum {
    kEepromAddrNodeIndex,  
    kEepromAddrState,     
    kEepromAddrColor,     
    kEepromAddrEffect,
} eepromAddr;

//Commands that can be received over RF
typedef enum {
    kRfCommandAddr,     //Set the node address of the module
    kRfCommandColor,    //Set a solid pixel color
    kRfCommandEffect,   //Set the leds to a preset effect
    kRfCommandOff,      //Set the leds off
    kRfCommandReset,    //Reset the receiver arduino
    kRfCommandNum,      //Number of commands
} rfCommand;

//State of the receiver node
typedef enum {
    kNodeStateColor,    //Sets leds to a single solid color
    kNodeStateEffect,   //Updates color based on preset effect
    kNodeStateOff,      //Sets all leds off
    kNodeStateReset,    //Resets the arduino
    kNodeStateNum,
} nodeState;

/**************************************************
 *          Global Variables & Objects 
 **************************************************/

RF24 radio(CE_PIN, CSN_PIN);
uint8_t node_index = MIN_NODE_INDEX;
const uint8_t master_address[BYTES_PER_ADDRESS] = "1Node";
nodeState state = kNodeStateOff;

uint8_t red = 0;    //Current value of the red pixel
uint8_t green = 0;  //Current value of the green pixel
uint8_t blue = 0;   //Current value of the blue pixel

effectList effect = 0;     //Current effect to run
effectList prev_effect = 0; //Effect run last cycle
bool effect_changed = true;  //Flag saying the effect has changed


/**************************************************
 *              Global Structs 
 **************************************************/

//Data packet received over RF radio
typedef struct {
    rfCommand command;  //Command to update state
    uint32_t data;      //Color or effect data
} rfPacket;

effectStruct *effects[] = {
    &rainbow,
    &fastRainbow,
    &halloween,
};

/**************************************************
 *              Global Functions 
 **************************************************/

/* RF Functions */

//Inits RF radio with default values
void initRF();
//Reads in the RF packet and updates state
int readRF(rfPacket &packet_p);    


/* LED Strip Functions */

//Updates the LED strip based on current state
void updateLights();        
//Updates the current effect running
void updateEffect();
//Returns the packed 32bit rgb value based on the given color         
uint32_t packColor(uint8_t r, uint8_t g, uint8_t b);
//Updates the given rgb values from the 32bit color given        
void unpackColor(uint32_t c, uint8_t &r, uint8_t &g, uint8_t &b);       

void setup() 
{
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);

    readEeprom();    

    initRF();

    //Serial.begin(SERIAL_BAUD);

    //TEST SETTINGS
    effect = kEffectHalloween;
    state = kNodeStateEffect;
    //red = 255;
    //green = 255;
    //blue = 255;
    //TEST SETTINGS
}

void loop()
{
    /* Read In Command From RF and update state */
    //readRF();

    /* Update LED strip based on current state */
    updateLights();

    /* Write values to EEPROM */ 
    writeEeprom();

    /* Reset arduino if state is RESET */
    if(state == kNodeStateReset)
    {
        reboot();
    }

    /* Wait a small delay before looping */
    delayMicroseconds(LOOP_DELAY);
} 

void reboot() 
{
    //Enable watchdog timer
    wdt_disable();
    wdt_enable(WDTO_15MS);
    //Loop indefinitely until reset
    while (true);
}

void readEeprom()
{
    int count;

    /* Read in last node index */
    node_index = EEPROM.read(kEepromAddrNodeIndex);
    if(node_index < MIN_NODE_INDEX || node_index > MAX_NODE_INDEX)
    {
        node_index = MIN_NODE_INDEX;
    }

    /* Read in last state */
    state = EEPROM.read(kEepromAddrState);
    if(state < 0 || state >= kNodeStateNum)
    {
        state = 0;
    }

    /* Read in last color */
    unpackColor(EEPROM.read(kEepromAddrColor), red, green, blue);

    /* Read in last effect */
    effect = EEPROM.read(kEepromAddrEffect);
    if(effect < 0 || effect >= kEffectNum)
    {
        effect = 0;
    }
}

void writeEeprom()
{
    EEPROM.write(kEepromAddrNodeIndex, node_index);
    EEPROM.write(kEepromAddrState, state);
    EEPROM.write(kEepromAddrColor, packColor(red, green, blue));
    EEPROM.write(kEepromAddrEffect, effect);
}

void initRF()
{
    /* Start the radio */
    radio.begin();
    radio.setAutoAck(1);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setRetries(0, MAX_RETRIES);
    radio.setPayloadSize(1);

    /* Set the address of the radio */
    uint8_t address[BYTES_PER_ADDRESS] = "2Node";
    address[0] = node_index + '0';
    radio.openReadingPipe(READING_PIPE, address);  

    /* Open the writing pipe on 0 to master */
    radio.openWritingPipe(master_address);
    radio.setPALevel(RF24_PA_MIN); 
    radio.startListening();
}

/******************************
 * readRF
 * Reads in radio packet and updates the state
 * Returns 1 if packet is read, 0 if none available,
 * and -1 if there was an error reading the data
 */
int readRF()
{
    //If there is a packet available, read it into struct
    if(radio.available())
    {
        rfPacket packet = {0};

        radio.read(&packet, sizeof(rfPacket));

        //Check that a valid command was read
        if(packet.command >= kRfCommandNum)
        {
            return -1;
        }

        //Update current state based on packet
        switch (packet.command)
        {
            case kRfCommandAddr:
                //TODO
                break;

            case kRfCommandColor:
                state = kNodeStateColor;
                //Update current color to that received
                unpackColor(packet.data, red, green, blue);
                break;

            case kRfCommandEffect:
                state = kNodeStateEffect;

                //Check that the effect index is valid
                if(packet.data >= kEffectNum)
                {
                    return -1;
                }

                //Update current effect to that received
                effect = packet.data;
                effect_changed = effect != prev_effect;
                break;

            case kRfCommandOff:
                state = kNodeStateOff;
                break;

            case kRfCommandReset:
                state = kNodeStateReset;
                break;

            case kRfCommandNum:
            default:
                return -1;
                break;
        }

        return 1;
    }

    return 0;
}


void updateLights()
{
    switch (state)
    {
        case kNodeStateColor:
            //Global color variables were already changed by readRF
            //So theres nothing to do here
            break;
        
        case kNodeStateEffect:          
            //Update the current time
            effects[effect]->config.time = (effects[effect]->config.use_ms) ? millis() : micros();
            //If the wait time has elapsed, increment count and run effect
            if(effects[effect]->config.time - effects[effect]->config.last >= effects[effect]->config.wait)
            {
                effects[effect]->config.last = effects[effect]->config.time;
                effects[effect]->config.count += effects[effect]->config.step;

                //Reset count if it has overflowed
                if(effects[effect]->config.count > effects[effect]->config.count_max)
                {
                    effects[effect]->config.count = 0;
                }

                //Run the current effect with the preset config
                effects[effect]->runEffect(effects[effect]->config);
            }

            //Update the previous effect to the current effect
            prev_effect = effect;
            break;

        case kNodeStateOff:
            //Set LEDS to off
            red = OFF;
            green = OFF;
            blue = OFF;
            break;
        
        case kNodeStateReset:
        case kNodeStateNum:
        default:
            break;
    }

    //Update pixels to current color
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
}


uint32_t packColor(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t c = 0;
    c |= (uint32_t)(r << RED_SHIFT);
    c |= (uint32_t)(g << GREEN_SHIFT);
    c |= (uint32_t)(b << BLUE_SHIFT);
    return c;
}


void unpackColor(uint32_t c, uint8_t &r, uint8_t &g, uint8_t &b)
{
    r = (uint8_t) (c >> RED_SHIFT);
    g = (uint8_t) (c >> GREEN_SHIFT);
    b = (uint8_t) (c >> BLUE_SHIFT);
}

/*uint32_t colorFade(uint16_t count, uint32_t c[], uint8_t length)
{
    uint8_t r1, r2, g1, g2, b1, b2;
    int index, next;

    index = count / (ONE_COLOR_COUNT_MAX + 1);
    next = (index < length) ? index + 1 : 0;

}*/

