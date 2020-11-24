#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>
#include <commonservices.h>

uint16_t ICACHE_FLASH_ATTR crc_xmodem_update (uint16_t crc, uint8_t data);

#endif