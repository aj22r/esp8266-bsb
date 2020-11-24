#ifndef _BSB_H_
#define _BSB_H_

#include <stdint.h>
#include <commonservices.h>

typedef struct bsb_telegram_t {
    uint8_t msg[32];
    uint8_t len;
} bsb_telegram_t;

int ICACHE_FLASH_ATTR bsb_process_byte(uint8_t b);
void ICACHE_FLASH_ATTR bsb_process_telegram(uint8_t* msg, int len);
bsb_telegram_t* ICACHE_FLASH_ATTR bsb_pop_telegram();
void ICACHE_FLASH_ATTR bsb_send(uint8_t src, uint8_t dest, uint8_t type, uint32_t cmd, uint8_t* payload, uint8_t payload_len);
void ICACHE_FLASH_ATTR bsb_init(int tx_pin);
void ICACHE_FLASH_ATTR bsb_poll();
uint8_t* ICACHE_FLASH_ATTR bsb_read_value_cached(uint32_t cmd, uint8_t* len);

#endif