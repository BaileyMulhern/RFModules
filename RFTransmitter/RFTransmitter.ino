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

#define MAX_RETRIES 15

#define NO_WAIT 0

#define MS_TO_S 1000

//Commands that can be received over RF
typedef enum {
    kRfCommandAddr,     //Set the node address of the module
    kRfCommandColor,    //Set a solid pixel color
    kRfCommandEffect,   //Set the leds to a preset effect
    kRfCommandOff,      //Set the leds off
    kRfCommandReset,    //Reset the receiver arduino
    kRfCommandNum,      //Number of commands
} rfCommand;

//List of all effects you can run 
typedef enum {
    kEffectRainbow, //RGB Rainbow
    kEffectFastRainbow,
    kEffectHalloween,
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
        uint8_t index;
        static uint8_t node_counter;
        static const uint8_t QUEUE_CAPACITY = 10;

        uint64_t last_update;

        rfPacket packet_q[QUEUE_CAPACITY];
        uint16_t wait_q[QUEUE_CAPACITY];
        uint8_t q_head;
        uint8_t q_tail;
        uint8_t q_count;

        bool dequeue();

    public:
        typedef enum {
            kMessageSent,
            kWriteFailed,
            kAckTimeout,
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


uint64_t g_millis = 0;
uint64_t g_last_button_debounce = 0;  

void initRF();

const int NUM_NODES = 2;
RFNode* nodes;

RF24 radio(CE_PIN, CSN_PIN);
const uint8_t talk_pipes[][BYTES_PER_ADDRESS] = {"1Talk", "2Talk", "3Talk", "4Talk", "5Talk"};
const uint8_t read_pipes[][BYTES_PER_ADDRESS] = {"1Read", "2Read", "3Read", "4Read", "5Read"};

const int DEMO_NUM = 3;
int demo_counter = 0;

rfPacket demo_reel[DEMO_NUM] = {
    {kRfCommandEffect, kEffectRainbow},
    {kRfCommandEffect, kEffectFastRainbow},
    {kRfCommandEffect, kEffectHalloween},
};

void setup() 
{
    //Setup the button
    pinMode(BUTTON_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ButtonISR, FALLING);

    nodes = (RFNode *)calloc(NUM_NODES, sizeof(RFNode));
    initRF();

    //Start serial line
    Serial.begin(SERIAL_BAUD);
}

void loop()
{
    g_millis = millis();

    for (int i = 0; i < NUM_NODES; i++)
    {
        nodes[i].update(g_millis);
    }

    delayMicroseconds(LOOP_DELAY);
}

void initRF()
{
    /* Start the radio */
    radio.begin();
    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();
    radio.setRetries(0, MAX_RETRIES);

    // Open writing pipe for all nodes
    for(int i = 0; i < NUM_NODES; i++)
    {
        radio.openReadingPipe(i + 1, (uint64_t)read_pipes[i]);
    }

    radio.setPALevel(RF24_PA_MIN); 
    radio.startListening();
}

RFNode::RFNode()
{
    q_head = 0;
    q_tail = 0;
    q_count = 0;

    index = node_counter++;
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
    q_head = ++q_head % QUEUE_CAPACITY;
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
        radio.stopListening();
        radio.openWritingPipe(talk_pipes[index]);

        //Return if the message sending failed
        if(!radio.write(&packet_q[q_head], sizeof(rfPacket)))
        {
            radio.startListening();
            return kWriteFailed;
        }

        radio.startListening();

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
    if(q_count == QUEUE_CAPACITY)
    {
        return false;
    }

    noInterrupts();

    wait_q[q_tail] = wait;
    packet_q[q_tail].command = p.command;
    packet_q[q_tail].data = p.data;

    q_tail = ++q_tail % QUEUE_CAPACITY;
    q_count++;

    interrupts();

    //Operation returns successfully
    return true;
}

/* ---------------------------------------------------------------------------------------------------- 
 *  INTERRUPT SERVICE ROUTINES
 */

void ButtonISR(void)
{
    if((millis() - g_last_button_debounce) >= DEBOUNCE_DELAY)
    {
        g_last_button_debounce = millis();
        
        for(int i = 0; i < NUM_NODES; i++)
        {
            nodes[i].queueMessage(demo_reel[demo_counter], NO_WAIT);
        }               
        
        demo_counter = ++demo_counter % DEMO_NUM;
    }
}
