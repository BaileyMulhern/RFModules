//#include <FastLED.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


#define CE_PIN      9
#define CSN_PIN     10
#define BUTTON_PIN  2

#define LOOP_DELAY  1
#define DEBOUNCE_DELAY 50
#define BYTES_PER_ADDRESS 6
#define SERIAL_BAUD 115200

#define NUM_NODES 5

#define NO_WAIT 0

//Commands that can be received over RF
typedef enum {
    kRfCommandAddr,     //Set the node address of the module
    kRfCommandColor,    //Set a solid pixel color
    kRfCommandEffect,   //Set the leds to a preset effect
    kRfCommandOff,      //Set the leds off
    kRfCommandReset,    //Reset the receiver arduino
    kRfCommandNum,      //Number of commands
} rfCommand;

//List of all effects 
typedef enum {
    kEffectRainbow, //RGB Rainbow
    kEffectFastRainbow,
    kEffectNum,     //Number of effects available
} effectList;

//Data packet sent over RF radio
typedef struct {
    rfCommand command;  //Command to update state
    uint32_t data;      //Color or effect data
} rfPacket;

class RFNode 
{
    private:
        uint8_t address[BYTES_PER_ADDRESS];
        uint8_t index;
        static uint8_t node_counter;
        static const uint8_t kIndexOffset = 2;
        static const uint8_t kQueueCapacity = 10;

        uint64_t last_update;

        rfPacket packet_q[kQueueCapacity];
        uint16_t wait_q[kQueueCapacity];
        uint8_t q_head;
        uint8_t q_tail;
        uint8_t q_count;

        bool dequeue();

    public:
        typedef enum {
            kMessageSent,
            kMessageFailed,
            kWait,
            kQueueEmpty,
        } updateStatus;

        RFNode();
        ~RFNode();
        updateStatus update(uint64_t ms);
        bool queueMessage(rfPacket p, uint16_t wait);
        int queueEffect(effectList e, uint16_t wait);
        int queueColor(uint32_t c, uint16_t wait);
};

RFNode::RFNode()
{
    q_head = 0;
    q_tail = 0;
    q_count = 0;

    index = kIndexOffset + node_counter++;

    address[0] = index + '0';
    address[1] = 'N';
    address[2] = 'o';
    address[3] = 'd';
    address[4] = 'e';
}

RFNode::~RFNode()
{
    delete[] packet_q;
}

bool RFNode::dequeue()
{
    //Return false if queue is empty
    if(q_count == 0)
    {
        return false;
    }

    noInterrupts();
    q_head = ++q_head % kQueueCapacity;
    q_count--;
    interrupts();

    //Return true if dequeued successfully
    return true;
}

RFNode::updateStatus RFNode::update(uint64_t ms)
{
    //Return false if there are no messages to send
    if(q_count == 0)
    {
        return kQueueEmpty;
    }

    if(ms - last_update >= wait_q[q_head])
    {
        /* TODO: Send rfPacket over rf */

        last_update = ms;
        dequeue();

        //Return true after successfully sending a packet
        return kMessageSent;
    }

    return kWait;
}

bool RFNode::queueMessage(rfPacket p, uint16_t wait = NO_WAIT)
{
    //Return false if queue is at capacity
    if(q_count == kQueueCapacity)
    {
        return false;
    }

    noInterrupts();

    wait_q[q_tail] = wait;
    packet_q[q_tail].command = p.command;
    packet_q[q_tail].data = p.data;

    q_tail = ++q_tail % kQueueCapacity;
    q_count++;

    interrupts();

    //Operation returns successfully
    return true;
}

RF24 radio(CE_PIN, CSN_PIN);
const uint8_t master_address[BYTES_PER_ADDRESS] = "1Node";

uint64_t g_millis = 0;
uint64_t g_last_button_debounce = 0;  

void initRF();

RFNode nodes[] = {

};

void setup() 
{
    //Setup the button
    pinMode(BUTTON_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonISR, FALLING);

    initRF();

    //Start serial line
    Serial.begin(SERIAL_BAUD);
}

void loop()
{
    g_millis = millis();

    for (RFNode n : nodes)
    {
        n.update(g_millis);
    }

    delayMicroseconds(LOOP_DELAY);
}

void initRF()
{
    /* TODO */

    // /* Start the radio */
    // radio.begin();
    // radio.setAutoAck(1);
    // radio.enableAckPayload();
    // radio.enableDynamicPayloads();
    // radio.setRetries(0, MAX_RETRIES);
    // radio.setPayloadSize(1);

    // /* Set the address of the radio */
    // uint8_t address[6] = "2Node";
    // address[0] = node_index + '0';
    // radio.openReadingPipe(READING_PIPE, address);  

    // /* Open the writing pipe on 0 to master */
    // radio.openWritingPipe(master_address);
    // radio.setPALevel(RF24_PA_MIN); 
    // radio.startListening();
}

/* ---------------------------------------------------------------------------------------------------- 
 *  INTERRUPT SERVICE ROUTINES
 */

void ButtonISR(void)
{
    if((millis() - g_last_button_debounce) >= DEBOUNCE_DELAY)
    {
        g_last_button_debounce = millis();
        
        /* TODO */
    }
}
