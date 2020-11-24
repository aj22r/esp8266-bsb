//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include <stdio.h>
#include "esp82xxutil.h"
#include "bsb.h"

static uint8_t hex2byte(char c) {
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len) {
	char * buffend = buffer;

	switch( pusrdata[1] ) {
		// Custom command test
		case 'C': case 'c':
			buffend += ets_sprintf( buffend, "CC" );
        	os_printf("CC");
			return buffend-buffer;

		// Echo to UART
		case 'E': case 'e':
			if( retsize <= len ) return -1;
			ets_memcpy( buffend, &(pusrdata[2]), len-2 );
			buffend[len-2] = '\0';
			os_printf( "%s\n", buffend );
			return len-2;

		case 'B': case 'b': {
			bsb_telegram_t* msg = bsb_pop_telegram();
			if(!msg) return -1;
			ets_memcpy(buffend, msg->msg, msg->len);
			return msg->len;
			//int i;
			//for(i = 0; i < msg->len; i++)
			//	buffend += os_sprintf(buffend, "%02X ", msg->msg[i]);
			//return buffend - buffer;
		}

		case 'T': case 't':
			//bsb_process_telegram("\xDC\x8A\x00\x05\xA5", 5);
			bsb_send(0x80, 0x0A, 6, 0x12345678, 0, 0);
			return -1;

		case 'S': case 's': {
			uint8_t src = ParamCaptureAndAdvanceInt();
			uint8_t dest = ParamCaptureAndAdvanceInt();
			uint8_t type = ParamCaptureAndAdvanceInt();
			uint32_t cmd = ParamCaptureAndAdvanceInt();
			uint8_t pl_len = ParamCaptureAndAdvanceInt();
			char* pl_hex = ParamCaptureAndAdvance();

			os_printf("Sending message, src: %02X, dst: %02X, type: %02X, cmd: %08X, payload: %s\r\n",
				src, dest, type, cmd, pl_hex);

			char payload[23];
			int i;
			for(i = 0; i < pl_len; i++) {
				payload[i] = (hex2byte(*pl_hex++) << 4) | hex2byte(*pl_hex++);
			}
			
			//for(i = 0; i < 2; i++)
			bsb_send(src, dest, type, cmd, payload, pl_len);
			return -1;
		}

		case 'R': case 'r': {
			uint32_t cmd = ParamCaptureAndAdvanceInt();
			uint8_t len;
			uint8_t* payload = bsb_read_value_cached(cmd, &len);
			if(!payload)
				return -1;
			ets_memcpy(buffend, payload, len);
			return len;
		}
	}

	return -1;
}
