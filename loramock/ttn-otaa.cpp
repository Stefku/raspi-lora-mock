#include "../../config.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
 
#include <lmic.h>
#include <hal/hal.h>

#include "ttn-otaa.h"
#include "command.cpp"

// APPEUI must be already defined
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 16);}

// DEVEUI must be already defined
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 16);}

// APPEKEY must be already defined
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 32);}

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty)
// cycle limitations).
const unsigned TX_INTERVAL = 60;

unsigned IS_JOINED = 0;
volatile unsigned scheduledTasks = 0;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = 0;

// Dragino Raspberry PI hat (no onboard led)
// see https://github.com/dragino/Lora
#define RF_CS_PIN  RPI_V2_GPIO_P1_22 // Slave Select on GPIO25 so P1 connector pin #22
#define RF_IRQ_PIN RPI_V2_GPIO_P1_07 // IRQ on GPIO4 so P1 connector pin #7
#define RF_RST_PIN RPI_V2_GPIO_P1_11 // Reset on GPIO17 so P1 connector pin #11

// Pin mapping
const lmic_pinmap lmic_pins = { 
    .nss  = RF_CS_PIN,
    .rxtx = LMIC_UNUSED_PIN,
    .rst  = RF_RST_PIN,
    .dio  = {LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
};

#ifndef RF_LED_PIN
#define RF_LED_PIN NOT_A_PIN  
#endif

void printTime() {
    char strTime[16];
    getSystemTime(strTime , sizeof(strTime));
    printf("%s: ", strTime);
}

void do_send(osjob_t* j) {
    char strTime[16];
    getSystemTime(strTime , sizeof(strTime));
    printf("%s: do_send scheduled tasks no %u with opmode:0x%04x - ", strTime, scheduledTasks, LMIC.opmode);
    scheduledTasks -= 1;

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        printf("OP_TXRXPEND: not sending\n");
        scheduleTask(TX_INTERVAL);
        return;
    }

    if (LMIC.opmode & OP_JOINING) {
        printf("OP_JOINING: device joining in progress\n");
        scheduleTask(TX_INTERVAL);
        return;
    }

    if (IS_JOINED == 0) {
        uint8_t joinData[] = "loramock";
        printf("try to send %d bytes while joining\n", sizeof(joinData));
        LMIC_setTxData2(1, joinData, sizeof(joinData), 0);
        scheduleTask(TX_INTERVAL);
        return;
    }

    std::ifstream ifs("command.txt");
    std::string content( (std::istreambuf_iterator<char>(ifs)),
                         (std::istreambuf_iterator<char>()) );

    if (content.size() <= 0) {
        printf("no command found. Retry in 5 seconds.\n");
        scheduleTask(5);
        return;
    }

    std::cout << "command found: ";
    std::cout << content << std::endl;

    Command command = parseCommand(content);
    if (command.valid == 0) {
        printf("no valid command found. Try [port]:[payload]\n");
        scheduleTask(5);
        std::remove("command.txt");
        return;
    }
    std::string payload = command.payload;

    uint8_t data[payload.size()];
    for (int i=0; i<payload.size(); i++) {
        data[i] = payload[i];
    }
    // Prepare upstream data transmission at the next possible time.
    printf("  try to send %s with %d bytes\n", payload, sizeof(data));
    LMIC_setTxData2(command.port, data, sizeof(data), 0);
    scheduleTask(TX_INTERVAL);

    printf("  Packet queued\n");
    std::remove("command.txt");
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    char strTime[16];
    getSystemTime(strTime , sizeof(strTime));
    printf("%s: ", strTime);
 
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            printf("EV_SCAN_TIMEOUT\n");
        break;
        case EV_BEACON_FOUND:
            printf("EV_BEACON_FOUND\n");
        break;
        case EV_BEACON_MISSED:
            printf("EV_BEACON_MISSED\n");
        break;
        case EV_BEACON_TRACKED:
            printf("EV_BEACON_TRACKED\n");
        break;
        case EV_JOINING:
            printf("EV_JOINING\n");
            IS_JOINED = 0;
        break;
        case EV_JOINED:
            printf("EV_JOINED\n");
            IS_JOINED = 1;
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
        break;
        case EV_RFU1:
            printf("EV_RFU1\n");
        break;
        case EV_JOIN_FAILED:
            printf("EV_JOIN_FAILED\n");
            IS_JOINED = 0;
        break;
        case EV_REJOIN_FAILED:
            printf("EV_REJOIN_FAILED\n");
            IS_JOINED = 0;
        break;
        case EV_TXCOMPLETE:
            printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
            if (LMIC.txrxFlags & TXRX_ACK) {
              printf("%s Received ack\n", strTime);
            }
            if (LMIC.dataLen) {
                  printf("%s Received %d bytes of payload\n", strTime, LMIC.dataLen);
                  uint8_t *ptr = LMIC.frame;
                  for (uint8_t i = 0; i < LMIC.dataLen; i++) {

                        printf("  Address of LMIC.frame[%u] = %x\n", i, ptr );
                        printf("  Value of LMIC.frame[%u] = %d\n", i, *ptr );

                        /* move to the next location */
                        ptr++;
                 }
            } else {
                printf("  no data.\n");
                scheduleTask(TX_INTERVAL);
            }
        break;
        case EV_LOST_TSYNC:
            printf("EV_LOST_TSYNC\n");
        break;
        case EV_RESET:
            printf("EV_RESET\n");
        break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            printf("EV_RXCOMPLETE\n");
        break;
        case EV_LINK_DEAD:
            printf("EV_LINK_DEAD\n");
        break;
        case EV_LINK_ALIVE:
            printf("EV_LINK_ALIVE\n");
        break;
        default:
            printf("Unknown event\n");
        break;
    }
}

void scheduleTask(unsigned intervall) {
    printTime();
    scheduledTasks++;
    /*
    if (scheduledTasks > 1) {
        // wait if a tasks currently started
        usleep(1000000);
    }
    if (scheduledTasks > 1) {
        printf("Don't schedule tasks, since %u tasks already scheduled.\n", scheduledTasks);
        return;
    }
    */
    printf("Schedule Task no %u with intervall %u\n", scheduledTasks, intervall);
    os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(intervall), do_send);
}

/* ======================================================================
Function: sig_handler
Purpose : Intercept CTRL-C keyboard to close application
Input   : signal received
Output  : -
Comments: -
====================================================================== */
void sig_handler(int sig) {
  printf("\nBreak received, exiting!\n");
  force_exit=true;
}

int main(void) {
    // caught CTRL-C to do clean-up
    signal(SIGINT, sig_handler);
    
    printf("%s Starting\n", __BASEFILE__);
    
      // Init GPIO bcm
    if (!bcm2835_init()) {
        fprintf( stderr, "bcm2835_init() Failed\n\n" );
        return 1;
    }

	// Show board config
    printConfig(RF_LED_PIN);
    printKeys();

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    scheduleTask(0);

    while(!force_exit) {
      os_runloop_once();
      
      // We're on a multitasking OS let some time for others
      // Without this one CPU is 99% and with this one just 3%
      // On a Raspberry PI 3
      usleep(1000);
    }

    // We're here because we need to exit, do it clean

    // module CS line High
    digitalWrite(lmic_pins.nss, HIGH);
    printf( "\n%s, done my job!\n", __BASEFILE__ );
    bcm2835_close();
    return 0;
}
