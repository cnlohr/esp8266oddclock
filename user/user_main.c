//Copyright 2015, 2018 <>< Charles Lohr, Adam Feinstein see LICENSE file.

/*==============================================================================
 * Includes
 *============================================================================*/

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "commonservices.h"
#include "vars.h"
#include <espnow.h>
#include "ws2812_i2s.h"
#include <mdns.h>
#include <string.h>
#include "nosdk8266.h"

//#define DOXMITX 1

/*==============================================================================
 * Process Defines
 *============================================================================*/

#define procTaskPrio        0
#define procTaskQueueLen    1
os_event_t    procTaskQueue[procTaskQueueLen];

const int devid = 28;

/*==============================================================================
 * Variables
 *============================================================================*/

static os_timer_t some_timer;
static struct espconn *pUdpServer;
usr_conf_t * UsrCfg = (usr_conf_t*)(SETTINGS.UserData);

/*==============================================================================
 * Functions
 *============================================================================*/



/**
 * This task is called constantly. The ESP can't handle infinite loops in tasks,
 * so this task will post to itself when finished, in essence looping forever
 *
 * @param events unused
 */
static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{
	CSTick( 0 );

	// Post the task in order to have it called again
	system_os_post(procTaskPrio, 0, 0 );
}

struct cnespsend
{
	uint32_t code;
	uint32_t op;
	uint32_t param1;
	uint32_t param2;
	uint8_t payload[12*3];
}  __attribute__ ((aligned (1))) __attribute__((packed));

/**
 * This is a timer set up in user_main() which is called every 100ms, forever
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR timerfn(void *arg)
{
static int lcode;
	static int k;
	k+=2;
	if( k > 150 ) k = 0;
	struct cnespsend thisesp;

		static int mode = 0;
//#if DOXMIT

	if( lcode == 0 || lcode == 1  || lcode == 2)
	{
		mode = 0;
		pico_i2c_writereg(103,4,1,0x88);	
		pico_i2c_writereg(103,4,2,0x91);	

	}
	if( lcode == 2048 || lcode == 2049 || lcode == 2050 )
	{
		pico_i2c_writereg(103,4,1,0x88);
		pico_i2c_writereg(103,4,2,0xf1);
		mode = 1;
	}

	if( lcode == 4096 || lcode == 4097 || lcode == 4098 )
	{
		pico_i2c_writereg(103,4,1,0x48);
		pico_i2c_writereg(103,4,2,0xf1);	
		mode = 2;
	}

	if( lcode == 2049 + 4096 ) lcode = 0;

	switch( mode )
	{
	case 0:			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0 ); break;
	case 1:			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1 ); break;
	case 2:			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), (lcode & 64)?1:0);

	}
//#endif

//			GPIO_OUTPUT_SET(GPIO_ID_PIN(2), (lcode & 1024)?1:0 );

	memset( &thisesp, 0, sizeof(thisesp) );
	thisesp.code = lcode++;
	thisesp.op = 3;
	thisesp.param1 = 66;
	thisesp.param2 = 900;
	thisesp.payload[0] = k;  //override
	thisesp.payload[1] = 160;
	thisesp.payload[2] = 0;
	thisesp.payload[3] = 175;
	thisesp.payload[4] = 100;

			//P1 is the offset
			//P2 is the multiplier
			//payload[0] = is the override if op is 4
			//payload[1] = max hue
			//payload[2] = flip hue (if true)
			//payload[3] = hue offset
			//payload[4] = bright mux

/*	int i;
	for( i = 0; i < 12; i++ )
	{
		uint32_t color = EHSVtoHEX( i*20, 255, 255 );
		thisesp.payload[i*3+0] = color>>8;
		thisesp.payload[i*3+1] = color;
		thisesp.payload[i*3+2] = color>>16;
	}*/

	uint8_t data[300];
#if DOXMITX
	espNowSend( data, 300 );
#endif
	CSTick( 1 ); // Send a one to uart
}

/**
 * UART RX handler, called by the uart task. Currently does nothing
 *
 * @param c The char received on the UART
 */
void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

/**
 * This is called on boot for versions ESP8266_NONOS_SDK_v1.5.2 to
 * ESP8266_NONOS_SDK_v2.2.1. system_phy_set_rfoption() may be called here
 */
void user_rf_pre_init(void)
{
	; // nothing
}

/**
 * Required function as of ESP8266_NONOS_SDK_v3.0.0. Must call
 * system_partition_table_regist(). This tries to register a few different
 * partition maps. The ESP should be happy with one of them.
 */
void ICACHE_FLASH_ATTR user_pre_init(void)
{
	LoadDefaultPartitionMap();
}




uint8_t lbuf[12*3];

/**
 * This callback function is called whenever an ESP-NOW packet is received
 *
 * @param mac_addr The MAC address of the sender
 * @param data     The data which was received
 * @param len      The length of the data which was received
 */
void ICACHE_FLASH_ATTR espNowRecvCb(uint8_t* mac_addr, uint8_t* data, uint8_t len)
{
    // Buried in a header, goes from 1 (far away) to 91 (practically touching)
    uint8_t rssi = data[-51];
	static uint8_t lastrssi;
	uint8_t totalrssi = rssi + lastrssi;
	lastrssi = rssi;


	printf( "%d  %d\n", rssi, data[0] );

#if 0
	struct cnespsend * d = (struct cnespsend*)data;


	if( d->code != 0xbeefbeef ) return;
	switch( d->op )
	{
	case 0:
		//printf( "Pushing %p %p\n", d->payload, data );
		ws2812_push( d->payload, 12*3 );
		break;
	case 1:
		ws2812_push( d->payload + 12*3*devid, 12*3 );
		break;
	case 2:
		if( d->param1 == devid )
			ws2812_push( d->payload, 12*3 );
		break;
	case 3:
	case 4:
		{
			int r = (d->op==3)?totalrssi:d->payload[0];
			int p1 = d->param1;
			int p2 = d->param2;


			if( p2 == 0 ) { printf( "NO P2\n" ); return; }
			int m = ((r - p1) * p2)>>8;

			int hue = m;
			if( hue < 0 ) hue = 0;
			if( hue > d->payload[1] ) hue = d->payload[1];
			if( d->payload[2] )
			{
				hue = d->payload[1] - 1 - hue;
			}
			hue += d->payload[3];

			int sat = ( 64 + d->payload[1] - m ) * 4;
			if( sat < 0 ) sat = 0;
			if( sat > 255 ) sat = 255;

			int val = (m + 64) * 4;
			if( val < 10 ) val = 10;
			if( val > 255 ) val = 255;
			uint32_t color = EHSVtoHEX( hue, sat, val );
			int i;
			for( i = 0; i < 12; i++ )
			{
				lbuf[i*3+0] = (((color>>8)&0xff) * d->payload[4])>>8;
				lbuf[i*3+1] = (((color)&0xff) * d->payload[4])>>8;
				lbuf[i*3+2] = (((color>>16)&0xff) * d->payload[4])>>8;
			}
			ws2812_push( lbuf, 12*3 );
		}
		break;
	}
#endif

#if 0
    // Debug print the received payload
    char dbg[256] = {0};
    char tmp[8] = {0};
    int i;
    for (i = 0; i < len; i++)
    {
        ets_sprintf(tmp, "%02X ", data[i]);
        strcat(dbg, tmp);
    }
    os_printf("%s, MAC [%02X:%02X:%02X:%02X:%02X:%02X], RSSI [%d], Bytes []\r\n",
              __func__,
              mac_addr[0],
              mac_addr[1],
              mac_addr[2],
              mac_addr[3],
              mac_addr[4],
              mac_addr[5],
              rssi );
#endif


}

/**
 * This is a wrapper for esp_now_send. It also sets the wifi power with
 * wifi_set_user_fixed_rate()
 *
 * @param data The data to broadcast using ESP NOW
 * @param len  The length of the data to broadcast
 */
void ICACHE_FLASH_ATTR espNowSend(const uint8_t* data, uint8_t len)
{
	/// This is the MAC address to transmit to for broadcasting
	static const uint8_t espNowBroadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Call this before each transmission to set the wifi speed
    wifi_set_user_fixed_rate(FIXED_RATE_MASK_ALL, 11); //3 = 11mbit/s B

    // Send a packet
   	esp_now_send((uint8_t*)espNowBroadcastMac, (uint8_t*)data, len);
}

/**
 * This callback function is registered to be called after an ESP NOW
 * transmission occurs. It notifies the program if the transmission
 * was successful or not. It gives no information about if the transmission
 * was received
 *
 * @param mac_addr The MAC address which was transmitted to
 * @param status   MT_TX_STATUS_OK or MT_TX_STATUS_FAILED
 */
void ICACHE_FLASH_ATTR espNowSendCb(uint8_t* mac_addr, uint8_t status)
{
    // os_printf("SEND MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    //           mac_addr[0],
    //           mac_addr[1],
    //           mac_addr[2],
    //           mac_addr[3],
    //           mac_addr[4],
    //           mac_addr[5]);

    switch(status)
    {
        case 0:
        {
            // os_printf("ESP NOW MT_TX_STATUS_OK\r\n");
            break;
        }
        default:
        {
            os_printf("ESP NOW MT_TX_STATUS_FAILED\r\n");
            break;
        }
    }
}

/**
 * Initialize ESP-NOW and attach callback functions
 */
void ICACHE_FLASH_ATTR espNowInit(void)
{
    if(0 == esp_now_init())
    {
        os_printf("ESP NOW init!\r\n");
        if(0 == esp_now_set_self_role(ESP_NOW_ROLE_COMBO))
        {
            os_printf("set as combo\r\n");
        }
        else
        {
            os_printf("esp now mode set fail\r\n");
        }

        if(0 == esp_now_register_recv_cb(espNowRecvCb))
        {
            os_printf("recvCb registered\r\n");
        }
        else
        {
            os_printf("recvCb NOT registered\r\n");
        }

        if(0 == esp_now_register_send_cb(espNowSendCb))
        {
            os_printf("sendCb registered\r\n");
        }
        else
        {
            os_printf("sendCb NOT registered\r\n");
        }
    }
    else
    {
        os_printf("esp now fail\r\n");
    }
}

/**
 * The default method, equivalent to main() in other environments. Handles all
 * initialization
 */
void ICACHE_FLASH_ATTR user_init(void)
{
	// Initialize the UART
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	os_printf("\r\nesp82XX Web-GUI\r\n%s\b", VERSSTR);

	//Uncomment this to force a system restore.
	//	system_restore();

	// Load settings and pre-initialize common services
	CSSettingsLoad( 0 );
	CSPreInit();

	ws2812_init();

	// Initialize common settings
	CSInit( 0 );

	// Start MDNS services
	SetServiceName( "espcom" );
	AddMDNSName(    "esp82xx" );
	AddMDNSName(    "espcom" );
	AddMDNSService( "_http._tcp",    "An ESP82XX Webserver", WEB_PORT );
	AddMDNSService( "_espcom._udp",  "ESP82XX Comunication", COM_PORT );
	AddMDNSService( "_esp82xx._udp", "ESP82XX Backend",      BACKEND_PORT );

/*
	struct softap_config {
		uint8 ssid[32];
		uint8 password[64];
		uint8 ssid_len;	// Note: Recommend to set it according to your ssid
		uint8 channel;	// Note: support 1 ~ 13
		AUTH_MODE authmode;	// Note: Don't support AUTH_WEP in softAP mode.
		uint8 ssid_hidden;	// Note: default 0
		uint8 max_connection;	// Note: default 4, max 4
		uint16 beacon_interval;	// Note: support 100 ~ 60000 ms, default 100
	} sap;*/
	struct softap_config sap;
	memset( &sap, 0, sizeof( sap ) );
	sap.ssid_len = 0;
	sap.channel = 6;
	sap.authmode = 0;
	sap.ssid_hidden = 1;
	sap.beacon_interval = 60000;

	wifi_softap_set_config(&sap);

	wifi_set_opmode( 2 );
	wifi_set_channel( 1 );

	// Set timer100ms to be called every 100ms
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)timerfn, NULL);
	os_timer_arm(&some_timer, 1, 1);

//This is actually 0x88.  You can set this to 0xC8 to double-overclock the low-end bus.
	//This can get you to a 160 MHz peripheral bus if setting it normally to 80 MHz.
#if PERIPH_FREQ == 160
//	pico_i2c_writereg(103,4,1,0xc8);
#else
//	pico_i2c_writereg(103,4,1,0x88);
#endif

	//@80, 20*32 ms = 640 * 2 = 1.280  BASE: 102.4
	//Now, 1.45 == 102.4/1.45 = ~70 MHz
	//5.09 s => 20 MHz	pico_i2c_writereg(103,4,1,0x08);	pico_i2c_writereg(103,4,2,0x91);		
	//40 MHz	pico_i2c_writereg(103,4,1,0x48);	pico_i2c_writereg(103,4,2,0x91);		
	//40 MHz	pico_i2c_writereg(103,4,1,0x48);	pico_i2c_writereg(103,4,2,0x91);		
	//pico_i2c_writereg(103,4,1,0x48);	pico_i2c_writereg(103,4,2,0x91);		

//	pico_i2c_writereg(103,4,1,0x88);	pico_i2c_writereg(103,4,2,0x91);		


	//Start the chip in slow-mode (1080 / 16) + no processor multiplier, so 32.5 MHz main system clock.
//		pico_i2c_writereg(103,4,1,0x48);
//		pico_i2c_writereg(103,4,2,0xf1);	


	os_printf( "Boot Ok.\n" );

	espNowInit();

	// Add a process and start it
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);
	system_os_post(procTaskPrio, 0, 0 );
}

/**
 * This will be called to disable any interrupts should the firmware enter a
 * section with critical timing. There is no code in this project that will
 * cause reboots if interrupts are disabled.
 */
void ICACHE_FLASH_ATTR EnterCritical(void)
{
	;
}

/**
 * This will be called to enable any interrupts after the firmware exits a
 * section with critical timing.
 */
void ICACHE_FLASH_ATTR ExitCritical(void)
{
	;
}
