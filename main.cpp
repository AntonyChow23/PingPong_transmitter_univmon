#include <time.h>
#include "mbed.h"
#include "sx1276-hal.h"
#include "univmon.h"

/* Set this flag to '1' to display debug messages on the console */
#define DEBUG_MESSAGE 1

/* LORA related macros */
#define RF_FREQUENCY 915000000  // Hz
#define TX_OUTPUT_POWER 20      // 14 dBm
#define LORA_BANDWIDTH \
    0                            // [0: 125 kHz,
                                 //  1: 250 kHz,
                                 //  2: 500 kHz,
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 7  // [SF7..SF12]
#define LORA_CODINGRATE \
    1                           // [1: 4/5,
                                //  2: 4/6,
                                //  3: 4/7,
                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 5   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_FHSS_ENABLED false
#define LORA_NB_SYMB_HOP 4
#define LORA_IQ_INVERSION_ON false
#define LORA_CRC_ENABLED true
/* LORA related macros end */

#define RX_TIMEOUT_VALUE 1000  // in ms
#define PAYLOAD_SIZE 20
#define BUFFER_SIZE 21  // Define the payload size here
#define PACKET_NUM_PER_GROUP 256
#define SEC_PER_MSG 1

/*
 *  Global variables declarations
 */
typedef enum {
    LOWPOWER = 0,
    IDLE,

    RX,
    RX_TIMEOUT,
    RX_ERROR,

    TX,
    TX_TIMEOUT,

    CAD,
    CAD_DONE
} AppStates_t;

volatile AppStates_t State = LOWPOWER;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*
 *  Global variables declarations
 */
SX1276MB1xAS Radio(NULL);

int count = 0;
int group = 0;
int total_distinct_count = 0;
int total_transmission_count = 0;
uint16_t PayloadSize = PAYLOAD_SIZE;
uint16_t send_buffersize = BUFFER_SIZE;
uint8_t send_buffer[BUFFER_SIZE];
uint16_t recv_buffersize = 0;
uint8_t recv_buffer[BUFFER_SIZE];
uint8_t temp_int[4];
int frequency = RF_FREQUENCY;
int16_t RssiValue = 0.0;
int8_t SnrValue = 0.0;
int recv_done = 0;
int recv_missing_list = 0;
uint8_t* payload_for_one_group[PACKET_NUM_PER_GROUP];
int stop_transmit_flag = 0;
int int_num_per_packet = PayloadSize / 4;

/* store current int to temp_int buffer */
void initialize_payload_for_one_group() {
    int i;
    for (i = 0; i < PACKET_NUM_PER_GROUP; i++) {
        payload_for_one_group[i] =
            (uint8_t*)calloc(send_buffersize, sizeof(uint8_t));
    }
}

void set_individual_payload_for_one_group(int num) {
//    debug("set_individual_payload_for_one_group\r\n");
//    debug("%d: ", num);
    int i;
    for (i = 0; i < send_buffersize; i++) {
        payload_for_one_group[num][i] = send_buffer[i];
//        debug("%d, ", payload_for_one_group[num][i]);
    }
//    debug("\r\n");
}

void set_send_buffer_from_payload_for_one_group(int num) {
//    debug("set_send_buffer_from_payload_for_one_group\r\n");
    int i;
    for (i = 0; i < send_buffersize; i++) {
        send_buffer[i] = payload_for_one_group[num][i];
//        debug("%d, ", send_buffer[i]);
    }
//    debug("\r\n");
}

void int_to_buf(int int_to_transmit) {
    temp_int[0] = (uint8_t)((int_to_transmit >> 24) & 0xFF);
    temp_int[1] = (uint8_t)((int_to_transmit >> 16) & 0xFF);
    temp_int[2] = (uint8_t)((int_to_transmit >> 8) & 0xFF);
    temp_int[3] = (uint8_t)(int_to_transmit & 0xFF);
}

void copy_temp_int_to_send_buffer(int temp_location) {
    int i;
    for (i = 0; i < 4; i++) {
        send_buffer[temp_location + i] = temp_int[i];
    }
}

void set_sendbuffer(int num, univSketch* univ, univTransmit* univ_transmit) {
    send_buffer[0] = num;
    int temp_location = 1;
    for (int i = 0; i < int_num_per_packet; i++) {
        int cur_value = get_int_from_cur_univmon_transmit(univ_transmit, univ);
        int_to_buf(cur_value);
        copy_temp_int_to_send_buffer(temp_location);
        temp_location += 4;
        if (update_univmon_transmit(univ_transmit, univ) == 1) {
            stop_transmit_flag = 1;
        }
    }
    set_individual_payload_for_one_group(num);

    /* debug purpose */
    for (int i = 0; i < send_buffersize; i++) {
        debug("%d ", send_buffer[i]);
    }
    debug("\r\n");
}

void send_sendbuffer() {
    wait_ms(500);
    Radio.Send(send_buffer, send_buffersize);
    debug("count: %d, size: %d\r\n", count, send_buffersize);
    count += 1;
    total_transmission_count += 1;
}

void set_and_send_sendbuffer(int num, univSketch* univ,
                             univTransmit* univ_transmit) {
    set_sendbuffer(num, univ, univ_transmit);
    send_sendbuffer();
}

void resend_packet(int num) {
    set_send_buffer_from_payload_for_one_group(num);
    send_sendbuffer();
}

void recv_proc_missing_list() {
    debug("receive missing list: ");
    int i;
    for (i = 0; i < recv_buffersize; i++) {
        if (recv_buffer[i] == PACKET_NUM_PER_GROUP - 1) {
            break;
        }
        debug("resend: %d\r\n", recv_buffer[i]);
        resend_packet(recv_buffer[i]);
    }
    recv_buffersize = 0;
}

void listen_and_resend() {
    clock_t start = clock();
    while (recv_done == 0) {
        wait_ms(30);
        Radio.Rx(RX_TIMEOUT_VALUE * 1000000);
        if (recv_missing_list == 0 &&
            (clock() - start) / (double)CLOCKS_PER_SEC > SEC_PER_MSG) {
            /* send last packet */
            resend_packet(PACKET_NUM_PER_GROUP - 1);
        } else if (recv_buffersize > 0 &&
                   recv_buffer[0] == PACKET_NUM_PER_GROUP - 1)
            break;
        else if (recv_buffersize > 0) {
            recv_proc_missing_list();
        }
    }
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size > 0) {
        recv_missing_list = 1;
        Radio.Sleep();
        recv_buffersize = size;
        memcpy(recv_buffer, payload, recv_buffersize);
        RssiValue = rssi;
        SnrValue = snr;
        State = RX;
        debug_if(DEBUG_MESSAGE, "> OnRxDone\n\r");
        int i;
        for (i = 0; i < recv_buffersize; i++) {
            debug("%d, ", recv_buffer[i]);
        }
        debug("\r\n");
    }
}

void OnTxDone(void) {
    Radio.Sleep();
    State = TX;
}

void OnTxTimeout(void) {
    Radio.Sleep();
    State = TX_TIMEOUT;
    debug_if(DEBUG_MESSAGE, "> OnTxTimeout\n\r");
}

void OnRxTimeout(void) {
    Radio.Sleep();
    recv_buffer[recv_buffersize] = 0;
    State = RX_TIMEOUT;
    debug_if(DEBUG_MESSAGE, "> OnRxTimeout\n\r");
}

void OnRxError(void) {
    Radio.Sleep();
    State = RX_ERROR;
    debug_if(DEBUG_MESSAGE, "> OnRxError\n\r");
}

int main(void) {
    // Initialize Radio driver
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.RxError = OnRxError;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    Radio.Init(&RadioEvents);

    // verify the connection with the board
    while (Radio.Read(REG_VERSION) == 0x00) {
        debug("Radio could not be detected!\n\r", NULL);
        wait(1);
    }

    debug_if((DEBUG_MESSAGE & (Radio.DetectBoardType() == SX1276MB1LAS)),
             "\n\r > Board Type: SX1276MB1LAS < \n\r");
    debug_if((DEBUG_MESSAGE & (Radio.DetectBoardType() == SX1276MB1MAS)),
             "\n\r > Board Type: SX1276MB1MAS < \n\r");

    Radio.SetChannel(RF_FREQUENCY);

    debug_if(LORA_FHSS_ENABLED, "\n\n\r             > LORA FHSS Mode < \n\n\r");
    debug_if(!LORA_FHSS_ENABLED, "\n\n\r             > LORA Mode < \n\n\r");

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      LORA_CRC_ENABLED, LORA_FHSS_ENABLED, LORA_NB_SYMB_HOP,
                      LORA_IQ_INVERSION_ON, 2000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0,
                      LORA_CRC_ENABLED, LORA_FHSS_ENABLED, LORA_NB_SYMB_HOP,
                      LORA_IQ_INVERSION_ON, true);

    Radio.SetChannel(frequency);

    clock_t start, end;
    start = clock();
    univSketch my_univmon;
    univSketch* univ = &my_univmon;
    init_univmon(univ);
    debug("start univmon\r\n");

    int i;
    for (i = 0; i < INPUT_NUM; i++) {
        univmon_processing(univ, (uint32_t)input[i]);
    }

    end = clock();
    debug("end univmon\r\n");
    debug("Processing duration: %lf\r\n", (end - start)/(double)CLOCKS_PER_SEC);


    univTransmit my_univmon_transmit;
    univTransmit* univ_transmit = &my_univmon_transmit;
    init_univmon_transmit(univ_transmit, univ);
    initialize_payload_for_one_group();
//    print_univmon(univ);

    debug("start transmitting %d\r\n", frequency);

    while (1) {
        set_and_send_sendbuffer(count, univ, univ_transmit);
        total_distinct_count += 1;
        if (stop_transmit_flag == 1) break;

        if (count % PACKET_NUM_PER_GROUP == 0) {
//            debug("payload_for_one_group\r\n");
//            for(int j = 0; j < 256; j++) {
//                debug("%d: ", j);
//                for(int k = 0; k < send_buffersize; k++) {
//                    debug("%d, ", payload_for_one_group[j][k]);
//                }
//                debug("\r\n");
//            }
            listen_and_resend();
            recv_done = 0;
            recv_buffersize = 0;
            recv_missing_list = 0;
            count = 0;

            /* test purpose */
            debug("TX group %d distinct %d transmit %d\r\n", group,
                  total_distinct_count, total_transmission_count);

            debug("start transmitting next group\r\n");
            group++;
        }
    }

    debug("end transmitting %d\r\n");

//    print_univmon_counter_heap(univ);
    debug("total distinct %d transmit %d\r\n", total_distinct_count, total_transmission_count);
}
