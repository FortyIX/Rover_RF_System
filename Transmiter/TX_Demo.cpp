#include "cc1100_raspi.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <getopt.h>
#include <ncurses.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <queue>



#include<SDL/SDL.h>

/* Function Prototypes */
void PrintKeyInfo( SDL_KeyboardEvent *key );
void PrintModifiers( SDLMod mod );

/* main */

/* Print all information about a key event */
void PrintKeyInfo( SDL_KeyboardEvent *key ){
	/* Is it a release or a press? */
	if( key->type == SDL_KEYUP )
		printf( "Release:- " );
	else
		printf( "Press:- " );

	/* Print the hardware scancode first */
	printf( "Scancode: 0x%02X", key->keysym.scancode );
	/* Print the name of the key */
	printf( ", Name: %s", SDL_GetKeyName( key->keysym.sym ) );
	/* We want to print the unicode info, but we need to make */
	/* sure its a press event first (remember, release events */
	/* don't have unicode info                                */
	if( key->type == SDL_KEYDOWN ){
		/* If the Unicode value is less than 0x80 then the    */
		/* unicode value can be used to get a printable       */
		/* representation of the key, using (char)unicode.    */
		printf(", Unicode: " );
		if( key->keysym.unicode < 0x80 && key->keysym.unicode > 0 ){
			printf( "%c (0x%04X)", (char)key->keysym.unicode,
					key->keysym.unicode );
		}
		else{
			printf( "? (0x%04X)", key->keysym.unicode );
		}
	}
	printf( "\n" );
	/* Print modifier info */
	PrintModifiers( key->keysym.mod );
}

/* Print modifier info */
void PrintModifiers( SDLMod mod ){
	printf( "Modifers: " );

	/* If there are none then say so and return */
	if( mod == KMOD_NONE ){
		printf( "None\n" );
		return;
	}

	/* Check for the presence of each SDLMod value */
	/* This looks messy, but there really isn't    */
	/* a clearer way.                              */
	if( mod & KMOD_NUM ) printf( "NUMLOCK " );
	if( mod & KMOD_CAPS ) printf( "CAPSLOCK " );
	if( mod & KMOD_LCTRL ) printf( "LCTRL " );
	if( mod & KMOD_RCTRL ) printf( "RCTRL " );
	if( mod & KMOD_RSHIFT ) printf( "RSHIFT " );
	if( mod & KMOD_LSHIFT ) printf( "LSHIFT " );
	if( mod & KMOD_RALT ) printf( "RALT " );
	if( mod & KMOD_LALT ) printf( "LALT " );
	if( mod & KMOD_CTRL ) printf( "CTRL " );
	if( mod & KMOD_SHIFT ) printf( "SHIFT " );
	if( mod & KMOD_ALT ) printf( "ALT " );
	printf( "\n" );
}
//https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideinputkeyboard.html 

#define PACKAGE    "CC1100 SW"
#define VERSION_SW "0.9.6"

#define INTERVAL_1S_TIMER 1000


//--------------------------[Global CC1100 variables]--------------------------
uint8_t Tx_fifo[256], Rx_fifo[FIFOBUFFER];
uint8_t My_addr, Tx_addr, Rx_addr, Lqi, Rssi;
uint8_t rx_addr,sender,lqi;
 int8_t rssi_dbm;

int cc1100_freq_select, cc1100_mode_select, cc1100_channel_select;

uint32_t prev_millis_1s_timer = 0;

uint8_t cc1100_debug = 0;								//set CC1100 lib in no-debug output mode
uint8_t tx_retries = 1;
uint8_t rx_demo_addr = 3;
int interval = 1000;

CC1100 cc1100;

//-------------------------- [End] --------------------------

void print_help(int exval) {
	printf("%s,%s by CW\r\n", PACKAGE, VERSION_SW);
	printf("%s [-h] [-V] [-a My_Addr] [-r RxDemo_Addr] [-i Msg_Interval] [-t tx_retries] [-c channel] [-f frequency]\r\n", PACKAGE);
	printf("          [-m modulation]\n\r\n\r");
	printf("  -h              			print this help and exit\r\n");
	printf("  -V              			print version and exit\r\n\r\n");
	printf("  -v              			set verbose flag\r\n");
	printf("  -a my address [1-255] 		set my address\r\n\r\n");
	printf("  -r rx address [1-255] 	  	set RxDemo receiver address\r\n\r\n");
	printf("  -i interval ms[1-6000] 	  	sets message interval timing\r\n\r\n");
	printf("  -t tx_retries [0-255] 	  	sets message send retries\r\n\r\n");
	printf("  -c channel 	[1-255] 		set transmit channel\r\n");
	printf("  -f frequency  [315,434,868,915]  	set ISM band\r\n\r\n");
	printf("  -m modulation [1,38,100,250,500,4]	set modulation\r\n\r\n");

	exit(exval);
}


//|============================ Main ============================|
int main(int argc, char *argv[]) {


	//------------- command line option parser -------------------
	int opt;
	// no arguments given
	if(argc == 1) {
		fprintf(stderr, "This program needs arguments....\n\r\n\r");
		print_help(1);
	}

	//TODO: Move to a new function, or just set the var exactly as in the Makefile

	while((opt = getopt(argc, argv, "hVva:r:i:t:c:f:m:")) != -1) {
		switch(opt) {
		case 'h':
			print_help(0);
			break;
		case 'V':
			printf("%s %s\n\n", PACKAGE, VERSION_SW);
			exit(0);
			break;
		case 'v':
			printf("%s: Verbose option is set `%c'\n", PACKAGE, optopt);
			cc1100_debug = 1;
			break;
		case 'a':
			My_addr = atoi (optarg);
			break;
		case 'r':
			rx_demo_addr = atoi (optarg);
			break;
		case 'i':
			interval = atoi (optarg);
			break;
		case 't':
			tx_retries = atoi (optarg);
			break;
		case 'c':
			cc1100_channel_select = atoi (optarg);
			break;
		case 'f':
			cc1100_freq_select = atoi (optarg);
			switch(cc1100_freq_select){
				case 315:
					cc1100_freq_select = 1;
					break;
				case 434:
					cc1100_freq_select = 2;
					break;
				case 868:
					cc1100_freq_select = 3;
					break;
				case 915:
					cc1100_freq_select = 4;
					break;
				}
			break;
			case 'm':
				cc1100_mode_select = atoi (optarg);

				switch(cc1100_mode_select){
				case 1:
					cc1100_mode_select = 1;
					break;
				case 38:
					cc1100_mode_select = 2;
					break;

				case 100:
					cc1100_mode_select = 3;
					break;
				case 250:
					cc1100_mode_select = 4;
					break;
				case 500:
					cc1100_mode_select = 5;
					break;
				case 4:
					cc1100_mode_select = 6;
					break;
				}
				break;
				case ':':
					fprintf(stderr, "%s: Error - Option `%c' needs a value\n\n", PACKAGE, optopt);
					print_help(1);
					break;
				case '?':
					fprintf(stderr, "%s: Error - No such option: `%c'\n\n", PACKAGE, optopt);
					print_help(1);
		}
	}

	// print all remaining options
	for(; optind < argc; optind++)
		printf("argument: %s\n", argv[optind]);

	
	
	//------------- welcome message ------------------------

	//------------- hardware setup ------------------------
	
	
	wiringPiSetup();			//setup wiringPi library

	cc1100.begin(My_addr);			//setup cc1000 RF IC
	cc1100.sidle();
	//cc1100.set_mode(0x04);                //set modulation mode 1 = GFSK_1_2_kb; 2 = GFSK_38_4_kb; 3 = GFSK_100_kb; 4 = MSK_250_kb; 5 = MSK_500_kb; 6 = OOK_4_8_kb
	//cc1100.set_ISM(0x03);                 //set ISM Band 1=315MHz; 2=433MHz; 3=868MHz; 4=915MHz
	//cc1100.set_channel(0x01);             //set channel
	cc1100.set_output_power_level(1);       //set PA level
	//cc1100.set_myaddr(0x05);
	
	cc1100.receive();
	cc1100.set_debug_level(1);

	cc1100.show_main_settings();             //shows setting debug messages to UART
	cc1100.show_register_settings();
	std::queue<uint8_t> keys;

	char a;
	int key = 0;
	//------------------------- Main Loop ------------------------
	SDL_Event event;
	int quit = 0;
	
	/* Initialise SDL */
	if( SDL_Init( SDL_INIT_VIDEO ) < 0){
		fprintf( stderr, "Could not initialise SDL: %s\n", SDL_GetError() );
		exit( -1 );
	}

	/* Set a video mode */
	if( !SDL_SetVideoMode( 320, 200, 0, 0 ) ){
		fprintf( stderr, "Could not set video mode: %s\n", SDL_GetError() );
		SDL_Quit();
		exit( -1 );
	}

	/* Enable Unicode translation */
	SDL_EnableUNICODE( 1 );
	uint8_t Pktlen = 8;
	uint8_t res = 0;
	/* Loop until an SDL_QUIT event is found */
	while( !quit ){

		/* Poll for events */
		while( SDL_PollEvent( &event ) ){
			
			switch( event.type ){
				/* Keyboard event */
				/* Pass the event data onto PrintKeyInfo() */
				case SDL_KEYDOWN:
					PrintKeyInfo( &event.key );
					Tx_fifo[3] = event.key.keysym.sym; 
					Tx_fifo[4] = 1;

					res = cc1100.sent_packet(My_addr, Rx_addr, Tx_fifo, Pktlen, tx_retries);
    				std::fill(Tx_fifo+3, Tx_fifo+254, 0); 	

					break;
				case SDL_KEYUP:
					
					PrintKeyInfo( &event.key );
					Tx_fifo[3] = event.key.keysym.sym; 
					Tx_fifo[4] = 0;

					res = cc1100.sent_packet(My_addr, Rx_addr, Tx_fifo, Pktlen, tx_retries);
    				std::fill(Tx_fifo+3, Tx_fifo+254, 0); 	
					
					break;

				/* SDL_QUIT event (window close) */
				case SDL_QUIT:
					quit = 1;
					break;

				default:
					break;
			}

		}

	}

	/* Clean up */
	SDL_Quit();
	exit( 0 );
	

    return 0;
}
//-------------------- END OF MAIN --------------------------------
