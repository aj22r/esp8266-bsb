#ifndef SOFTUART_H_
#define SOFTUART_H_

#include "user_interface.h"

#define SOFTUART_GPIO_COUNT 16

typedef struct softuart_pin_t {
	uint8_t gpio_id;
	uint32_t gpio_mux_name;
	uint8_t gpio_func;
} softuart_pin_t;

typedef struct {
	softuart_pin_t pin_tx;
	uint16_t bit_time;
} Softuart;


BOOL ICACHE_FLASH_ATTR Softuart_Available(Softuart *s);
void ICACHE_FLASH_ATTR Softuart_SetPinTx(Softuart *s, uint8_t gpio_id);
void ICACHE_FLASH_ATTR Softuart_Init(Softuart *s, uint32_t baudrate);
void ICACHE_FLASH_ATTR Softuart_Putchar(Softuart *s, char data);
void ICACHE_FLASH_ATTR Softuart_Puts(Softuart *s, const char *c );

//define mapping from pin to functio mode
typedef struct {
	uint32_t gpio_mux_name;
	uint8_t gpio_func;
} softuart_reg_t;

#endif /* SOFTUART_H_ */