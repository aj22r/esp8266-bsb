#include "bsb.h"
#include "crc.h"
#include "uart.h"
#include "softuart.h"

#define NUM_TELEGRAM_BUFFER 8
#define BSB_CACHE_SIZE 32
#define BSB_CACHE_TIMEOFLIFE (90*1000000) // 90 seconds (in microseconds)
#define BSB_CACHE_REQUESTINTERVAL (2*1000000) // 2 seconds (in microseconds)
#define BSB_SELF_ADDR 66

static bsb_telegram_t telegram_buffer[NUM_TELEGRAM_BUFFER]; // can hold 8 messages at once
static int telegram_buffer_head = 0;
static int telegram_buffer_tail = 0;
static bool telegram_buffer_full = false;
static int current_buffer_idx = 0;

static Softuart softuart;

static struct {
    uint32_t cmd;
    uint8_t payload[23];
    uint8_t payload_len;
    uint32_t last_recv;
} bsb_cache[BSB_CACHE_SIZE];

// Generates CCITT XMODEM CRC from BSB message
static uint16_t ICACHE_FLASH_ATTR bsb_crc(uint8_t* buffer, uint8_t length) {
    uint16_t crc = 0;
    while(length--)
        crc = crc_xmodem_update(crc, *buffer++);
    // Complete message returns 0x00
    // Message w/o last 2 bytes (CRC) returns last 2 bytes (CRC)
    return crc;
}

void ICACHE_FLASH_ATTR bsb_process_telegram(uint8_t* msg, int len) {
    if(len >= 11) { // packet + crc
        uint8_t msg_type = msg[4];
        
        // only accept answer type
        if(msg_type == 7) {
            uint32_t cmd;
            if(msg_type == 3 || msg_type == 6) { // QUERY and SET: byte 5 and 6 are in reversed order
                cmd = ((uint32_t)msg[6]<<24) | ((uint32_t)msg[5]<<16) | ((uint32_t)msg[7]<<8) | (uint32_t)msg[8];
            } else {
                cmd = ((uint32_t)msg[5]<<24) | ((uint32_t)msg[6]<<16) | ((uint32_t)msg[7]<<8) | (uint32_t)msg[8];
            }

            int i;
            for(i = 0; i < BSB_CACHE_SIZE; i++) {
                if(bsb_cache[i].cmd == cmd) {
                    bsb_cache[i].payload_len = len - 11;
                    ets_memcpy(bsb_cache[i].payload, msg + 9, len - 11);
                    bsb_cache[i].last_recv = system_get_time();
                    break;
                }
            }
        }
    }

    ets_bzero(telegram_buffer[telegram_buffer_head].msg, 32);
    telegram_buffer[telegram_buffer_head].len = len;
    ets_memcpy(telegram_buffer[telegram_buffer_head].msg, msg, len);

    telegram_buffer_head = (telegram_buffer_head + 1) % NUM_TELEGRAM_BUFFER;
    if(telegram_buffer_full) {
        telegram_buffer_tail = (telegram_buffer_tail + 1) % NUM_TELEGRAM_BUFFER;
    } else {
        if(telegram_buffer_head == telegram_buffer_tail)
            telegram_buffer_full = true;
    }
}

static uint32_t last_recv = 0;
int ICACHE_FLASH_ATTR bsb_process_byte(uint8_t b) {
    static uint8_t buf[32];
	static int buf_idx = 0;

    if(buf_idx > 0) {
        // timeout after 5ms
        if((system_get_time() & 0x7fffffff) - last_recv > 5000)
            buf_idx = 0;
    }
    last_recv = system_get_time() & 0x7fffffff;

    // Check for valid starting byte
    if(buf_idx == 0)
        if(b != 0xDC && b != 0xDE)
            return 1;

    buf[buf_idx++] = b;

    if(buf_idx > 3) {
        uint8_t msg_len = buf[3];
        if(msg_len > sizeof(buf)) {
            buf_idx = 0;
            return 2;
        }
        if(buf_idx >= msg_len) {
            int ret = 0;
            if(bsb_crc(buf, buf_idx) == 0)
                bsb_process_telegram(buf, buf_idx);
            else
                ret = 3;
            buf_idx = 0;
            return ret;
        }
    }

    return 0;
}

bsb_telegram_t* ICACHE_FLASH_ATTR bsb_pop_telegram() {
    bsb_poll();

    if(telegram_buffer_head == telegram_buffer_tail && !telegram_buffer_full)
        return 0;
    telegram_buffer_full = false;
    bsb_telegram_t* buf = &telegram_buffer[telegram_buffer_tail++];
    telegram_buffer_tail %= NUM_TELEGRAM_BUFFER;
    //os_printf("Tail: %d, head: %d\n", telegram_buffer_tail, telegram_buffer_head);
    return buf;
}

void ICACHE_FLASH_ATTR bsb_send(uint8_t src, uint8_t dest, uint8_t type, uint32_t cmd, uint8_t* payload, uint8_t payload_len) {
    if(payload_len > 32 - 9)
        return;

    uint8_t msg[32];
    msg[0] = 0xDC;
    msg[1] = src | 0x80;
    msg[2] = dest;
    msg[3] = 9 + payload_len + 2; // packet + payload + crc
    msg[4] = type;
    msg[6] = (cmd & 0xff000000) >> 24;
    msg[5] = (cmd & 0x00ff0000) >> 16;
    msg[7] = (cmd & 0x0000ff00) >> 8;
    msg[8] = (cmd & 0x000000ff);
    ets_memcpy(msg + 9, payload, payload_len);
    uint16_t crc = bsb_crc(msg, 9 + payload_len);
    msg[9 + payload_len] = crc >> 8;
    msg[9 + payload_len + 1] = crc & 0xFF;

    ETS_INTR_LOCK();
    int i;
    for(i = 0; i < msg[3]; i++) {
        //os_printf("sending byte: %02X\r\n", msg[i]);
        //uart_tx_one_char(UART1, msg[i] ^ 0xFF);
        Softuart_Putchar(&softuart, msg[i] ^ 0xFF);
    }
    ETS_INTR_UNLOCK();
}

void ICACHE_FLASH_ATTR bsb_init(int tx_pin) {
    ets_bzero(bsb_cache, sizeof(bsb_cache));

    uart_init(BIT_RATE_4800, BIT_RATE_4800);
	UART_SetParity(UART0, ODD_BITS);
	UART_SetParity(UART1, ODD_BITS);

    Softuart_SetPinTx(&softuart, tx_pin);
    Softuart_Init(&softuart, 4800);
}

void ICACHE_FLASH_ATTR bsb_poll() {
    char b;
	while(rx_buff_deq(&b, 1)) {
		//uart_tx_one_char(UART0, b);
        bsb_process_byte(b ^ 0xFF);
        //if(bsb_process_byte(b ^ 0xFF) != 0)
        //    bsb_process_telegram("\xDC\xFF\xFF\x05\xFF", 5);
        //os_printf("processing byte: %02X, %d\r\n", b ^ 0xFF, bsb_process_byte(b ^ 0xFF));
	}
}

static uint32_t last_request = 0;

uint8_t* ICACHE_FLASH_ATTR bsb_read_value_cached(uint32_t cmd, uint8_t* len) {
    int slot_idx = -1;
    int empty_slot_idx = -1;
    int i;
    for(i = 0; i < BSB_CACHE_SIZE; i++) {
        if(bsb_cache[i].cmd == cmd) {
            slot_idx = i;
            break;
        } else if(bsb_cache[i].cmd == 0 && empty_slot_idx == -1) {
            empty_slot_idx = i;
        }
    }

    if(slot_idx == -1 && empty_slot_idx == -1)
        return 0;
    
    int slot = slot_idx == -1 ? empty_slot_idx : slot_idx;
    os_printf("Found slot %d for cmd %08X\n", slot, cmd);

    if(slot_idx == -1 ||
        (system_get_time() - bsb_cache[slot].last_recv > BSB_CACHE_TIMEOFLIFE &&
        system_get_time() - last_request > BSB_CACHE_REQUESTINTERVAL))
    {
        bsb_cache[slot].cmd = cmd;
        last_request = system_get_time();
        bsb_send(BSB_SELF_ADDR, 0, 6, cmd, 0, 0);
        os_printf("Querying cmd\n");
    }

    if(slot_idx != -1) {
        *len = bsb_cache[slot].payload_len;
        os_printf("Returning payload with len %d\n", *len);
        return bsb_cache[slot].payload;
    }
    return 0;
}