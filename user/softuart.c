  
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "softuart.h"

//array of pointers to instances
Softuart *_Softuart_GPIO_Instances[SOFTUART_GPIO_COUNT];
uint8_t _Softuart_Instances_Count = 0;

//intialize list of gpio names and functions
softuart_reg_t softuart_reg[] =
{
	{ PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0 },	//gpio0
	{ PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1 },	//gpio1 (uart)
	{ PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2 },	//gpio2
	{ PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3 },	//gpio3 (uart)
	{ PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4 },	//gpio4
	{ PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 },	//gpio5
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12 },	//gpio12
	{ PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13 },	//gpio13
	{ PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14 },	//gpio14
	{ PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15 },	//gpio15
	//@TODO TODO gpio16 is missing (?include)
};

uint8_t ICACHE_FLASH_ATTR Softuart_Bitcount(uint32_t x)
{
	uint8_t count;
 
	for (count=0; x != 0; x>>=1) {
		if ( x & 0x01) {
			return count;
		}
		count++;
	}
	//error: no 1 found!
	return 0xFF;
}

uint8_t ICACHE_FLASH_ATTR Softuart_IsGpioValid(uint8_t gpio_id)
{
	if ((gpio_id > 5 && gpio_id < 12) || gpio_id > 15)
	{
		return 0;
	}
	return 1;
}

void ICACHE_FLASH_ATTR Softuart_SetPinTx(Softuart *s, uint8_t gpio_id)
{ 
	if(! Softuart_IsGpioValid(gpio_id)) {
		os_printf("SOFTUART GPIO not valid %d\r\n",gpio_id);
	} else {
		s->pin_tx.gpio_id = gpio_id;
		s->pin_tx.gpio_mux_name = softuart_reg[gpio_id].gpio_mux_name;
		s->pin_tx.gpio_func = softuart_reg[gpio_id].gpio_func;
	}
}

void ICACHE_FLASH_ATTR Softuart_Init(Softuart *s, uint32_t baudrate)
{

	if(! _Softuart_Instances_Count) {
		os_printf("SOFTUART initialize gpio\r\n");
		//Initilaize gpio subsystem
		gpio_init();
	}

	//set bit time
	if(baudrate <= 0) {
            os_printf("SOFTUART ERROR: Set baud rate (%d)\r\n",baudrate);
    } else {
        s->bit_time = (1000000 / baudrate);
        if ( ((100000000 / baudrate) - (100*s->bit_time)) > 50 ) s->bit_time++;
        os_printf("SOFTUART bit_time is %d\r\n",s->bit_time);
    }


	//init tx pin
	if(!s->pin_tx.gpio_mux_name) {
		os_printf("SOFTUART ERROR: Set tx pin (%d)\r\n",s->pin_tx.gpio_mux_name);
	} else {
		//enable pin as gpio
    	PIN_FUNC_SELECT(s->pin_tx.gpio_mux_name, s->pin_tx.gpio_func);

		//set pullup (UART idle is VDD)
		PIN_PULLUP_EN(s->pin_tx.gpio_mux_name);
		
		//set high for tx idle
		GPIO_OUTPUT_SET(GPIO_ID_PIN(s->pin_tx.gpio_id), 1);
		os_delay_us(100000);
		
		os_printf("SOFTUART TX INIT DONE\r\n");
	}

	//add instance to array of instances
	_Softuart_GPIO_Instances[s->pin_tx.gpio_id] = s;
	_Softuart_Instances_Count++;
		
	os_printf("SOFTUART INIT DONE\r\n");
}

static inline u8 chbit(u8 data, u8 bit)
{
    if ((data & bit) != 0)
    {
    	return 1;
    }
    else
    {
    	return 0;
    }
}

// Function for printing individual characters
void ICACHE_FLASH_ATTR Softuart_Putchar(Softuart *s, char data)
{
	unsigned i;
	unsigned start_time = 0x7FFFFFFF & system_get_time();
    uint8_t parity = 1;

	//Start Bit
	GPIO_OUTPUT_SET(GPIO_ID_PIN(s->pin_tx.gpio_id), 0);
	for(i = 0; i < 8; i++ )
	{
		while ((0x7FFFFFFF & system_get_time()) < (start_time + (s->bit_time*(i+1))))
		{
			//If system timer overflow, escape from while loop
			if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
		}
		GPIO_OUTPUT_SET(GPIO_ID_PIN(s->pin_tx.gpio_id), data & 1);
        parity ^= data & 1;
        data >>= 1;
	}

    // Parity bit
	while ((0x7FFFFFFF & system_get_time()) < (start_time + (s->bit_time*9)))
	{
		//If system timer overflow, escape from while loop
		if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
	}
	GPIO_OUTPUT_SET(GPIO_ID_PIN(s->pin_tx.gpio_id), parity);

	// Stop bit
	while ((0x7FFFFFFF & system_get_time()) < (start_time + (s->bit_time*10)))
	{
		//If system timer overflow, escape from while loop
		if ((0x7FFFFFFF & system_get_time()) < start_time){break;}
	}
	GPIO_OUTPUT_SET(GPIO_ID_PIN(s->pin_tx.gpio_id), 1);

	// Delay after byte, for new sync
	os_delay_us(s->bit_time*6);
}

void ICACHE_FLASH_ATTR Softuart_Puts(Softuart *s, const char *c )
{
	while ( *c ) {
      	Softuart_Putchar(s,( u8 )*c++);
	}
}