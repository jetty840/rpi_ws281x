/*
 * newtest.c
 *
 * Copyright (c) 2014 Jeremy Garff <jer @ jers.net>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     1.  Redistributions of source code must retain the above copyright notice, this list of
 *         conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright notice, this list
 *         of conditions and the following disclaimer in the documentation and/or other materials
 *         provided with the distribution.
 *     3.  Neither the name of the owner nor the names of its contributors may be used to endorse
 *         or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


static char VERSION[] = "XX.YY.ZZ";

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#include "clk.h"
#include "gpio.h"
#include "dma.h"
#include "pwm.h"
#include "animations.h"
#include "version.h"

#include "ws2811.h"


#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     5
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GBR		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)

#define WIDTH                   16
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)

#define PORT 9999

int width = WIDTH;
int height = HEIGHT;
int led_count = LED_COUNT;

int clear_on_exit = 0;

int port = PORT;

#define BUFSIZE 1024



ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

ws2811_led_t *matrix;

volatile static uint8_t running = 1;

static void ctrl_c_handler(int signum)
{
    running = 0;
}

static void setup_handlers(void)
{
    struct sigaction sa =
    {
        .sa_handler = ctrl_c_handler,
    };

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


void parseargs(int argc, char **argv, ws2811_t *ws2811)
{
	int index;
	int c;

	static struct option longopts[] =
	{
		{"help", no_argument, 0, 'h'},
		{"dma", required_argument, 0, 'd'},
		{"gpio", required_argument, 0, 'g'},
		{"invert", no_argument, 0, 'i'},
		{"clear", no_argument, 0, 'c'},
		{"strip", required_argument, 0, 's'},
		{"height", required_argument, 0, 'y'},
		{"width", required_argument, 0, 'x'},
		{"version", no_argument, 0, 'v'},
		{"port", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	while (1)
	{

		index = 0;
		c = getopt_long(argc, argv, "cd:g:hip:s:vx:y:", longopts, &index);

		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			/* handle flag options (array's 3rd field non-0) */
			break;

		case 'h':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			fprintf(stderr, "Usage: %s \n"
				"-h (--help)    - this information\n"
				"-s (--strip)   - strip type - rgb, grb, gbr, rgbw\n"
				"-x (--width)   - matrix width (default 8)\n"
				"-y (--height)  - matrix height (default 8)\n"
				"-d (--dma)     - dma channel to use (default 5)\n"
				"-g (--gpio)    - GPIO to use\n"
				"                 If omitted, default is 18 (PWM0)\n"
				"-i (--invert)  - invert pin output (pulse LOW)\n"
				"-p (--port)    - udp port to listen on (default 9999)\n"
				"-c (--clear)   - clear matrix on exit.\n"
				"-v (--version) - version information\n"
				, argv[0]);
			exit(-1);

		case 'D':
			break;

		case 'g':
			if (optarg) {
				int gpio = atoi(optarg);
/*
	PWM0, which can be set to use GPIOs 12, 18, 40, and 52. 
	Only 12 (pin 32) and 18 (pin 12) are available on the B+/2B/3B
	PWM1 which can be set to use GPIOs 13, 19, 41, 45 and 53. 
	Only 13 is available on the B+/2B/PiZero/3B, on pin 33
	PCM_DOUT, which can be set to use GPIOs 21 and 31.
	Only 21 is available on the B+/2B/PiZero/3B, on pin 40.
	SPI0-MOSI is available on GPIOs 10 and 38.
	Only GPIO 10 is available on all models.

	The library checks if the specified gpio is available
	on the specific model (from model B rev 1 till 3B)

*/
				ws2811->channel[0].gpionum = gpio;
			}
			break;

		case 'i':
			ws2811->channel[0].invert=1;
			break;

		case 'c':
			clear_on_exit=1;
			break;

		case 'd':
			if (optarg) {
				int dma = atoi(optarg);
				if (dma < 14) {
					ws2811->dmanum = dma;
				} else {
					fprintf (stderr, "invalid dma %d\n", dma);
					exit (-1);
				}
			}
			break;

		case 'p':
			if (optarg) {
				port = atoi(optarg);
				if ( port < 1 )
				{
					fprintf (stderr, "invalid port %d\n", port);
					exit (-1);
				}
			}
			break;

		case 'y':
			if (optarg) {
				height = atoi(optarg);
				if (height > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					fprintf (stderr, "invalid height %d\n", height);
					exit (-1);
				}
			}
			break;

		case 'x':
			if (optarg) {
				width = atoi(optarg);
				if (width > 0) {
					ws2811->channel[0].count = height * width;
				} else {
					fprintf (stderr, "invalid width %d\n", width);
					exit (-1);
				}
			}
			break;

		case 's':
			if (optarg) {
				if (!strncasecmp("rgb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RGB;
				}
				else if (!strncasecmp("rbg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_RBG;
				}
				else if (!strncasecmp("grb", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GRB;
				}
				else if (!strncasecmp("gbr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_GBR;
				}
				else if (!strncasecmp("brg", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BRG;
				}
				else if (!strncasecmp("bgr", optarg, 4)) {
					ws2811->channel[0].strip_type = WS2811_STRIP_BGR;
				}
				else if (!strncasecmp("rgbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_RGBW;
				}
				else if (!strncasecmp("grbw", optarg, 4)) {
					ws2811->channel[0].strip_type = SK6812_STRIP_GRBW;
				}
				else {
					fprintf (stderr, "invalid strip %s\n", optarg);
					exit (-1);
				}
			}
			break;

		case 'v':
			fprintf(stderr, "%s version %s\n", argv[0], VERSION);
			exit(-1);

		case '?':
			/* getopt_long already reported error? */
			exit(-1);

		default:
			exit(-1);
		}
	}
}



void callInitAnimationFunction(int animationId)
{
    // Call the init function
    if ( animations[animationId].fps == 0 )	sleepTime = 0UL;
    else					sleepTime = 1000000UL / (unsigned long)animations[animationId].fps;

    if ( animations[animationId].initFunc )	animations[animationId].initFunc(animations[animationId].param1, animations[animationId].param2, animations[animationId].param3);
}



void callIterateAnimationFunction(int animationId)
{
	// Call the iterate function
	if ( animations[animationId].iterateFunc )	animations[animationId].iterateFunc(animations[animationId].param1, animations[animationId].param2, animations[animationId].param3);
}



// Starts up the udp server, returns the socket file description on success, otherwise -1

int start_udp_server(void)
{
    int sockfd;
    int optval;
    struct sockaddr_in serveraddr;
    struct timeval read_timeout;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0 );
    if ( sockfd < 0 )
    {
	perror("unable to create socket");
	return -1;
    }

    // So we can restart the server without waiting
    optval = 1;
    if ( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(optval)) )
    {
	perror("unable to set socket option SO_REUSEADDR");
	close(sockfd);
	return -1;
    }

    // Set a timeout of 1uS so that recvfrom becomes non blocking
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1;
    if ( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) )
    {
	perror("unable to set socket option SO_RCVTIMEO");
	close(sockfd);
	return -1;
    }

    // bind a listening port to the socket
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port); 

    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
    {
	perror("unable to bind to socket");
	close(sockfd);
	return -1;
    }

    return sockfd;
}



// Attempts to receive a packet on the udp socket
// Provide socket fd, buffer and size of buffer to put received packet in
// Return > 0 = number of bytes received
// Return == 0 = Empty transmission or timed out
// Return < 0 = error

int receive_udp_packet(int sockfd, char *buf, size_t bufSize)
{
	int n;

	n = recvfrom(sockfd, buf, bufSize, 0, NULL, NULL);
	if ( n < 0 && errno != EAGAIN )
	{
		perror("recvfrom");
		return -1;
	}

	// This is for the situation where there was no data and we timed out, everything okay,
	// but the data is zero length
	if ( n < 0 )	n = 0;

	// Terminate the string
	buf[n] = '\0';

	return n;
}



// Provide an animation name, and it returns the animation Id, returns -1 if not found

int animationIdByName(char *name)
{
	int i,t;

	// search for the request
	for ( i = 0; animations[i].initFunc; i ++ )
	{
		for ( t = 0; animations[i].names[t]; t ++ )
		{
			if ( strcmp(name, animations[i].names[t]) == 0 )	return i;
		}
	}

	return -1;
}




// Waits for a packet to be received on sockfd, or times out after timeout(uS) if one is not recevied
// If timeout is 0, then it returns after a maximum of 1 second (depending on if a packet is recevied or not)
// 
// Returns -1 on error
// Returns 0 if no data is recevied during the timeout
// Returns 1 if data is recevied during the timeout

int wait_for_packet_or_timeout(int sockfd, unsigned long timeout)
{
	int n;
	fd_set rfds;
	struct timeval tv;

	tv.tv_sec  = ( timeout ) ? 0	   : 1;
	tv.tv_usec = ( timeout ) ? timeout : 0;

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	n = select(sockfd+1, &rfds, NULL, NULL, &tv);

	if ( n < 0 )	perror("select failed");

	return n;
}



int animationRender(void)
{
	int i, ret;

	for (i = 0; i < width; i ++ )	ledstring.channel[0].leds[i] = matrix[i];

        if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
        {
		fprintf(stderr, "ws2811_render failed: %s\n", ws2811_get_return_t_str(ret));
		return -1;
        }

	return 0;
}
		


int main(int argc, char *argv[])
{
    int sockfd;
    ws2811_return_t ret;
    int activeAnimation = 1;
    char buf[BUFSIZE + 1];

    sprintf(VERSION, "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

    parseargs(argc, argv, &ledstring);

    matrix = malloc(sizeof(ws2811_led_t) * width * height);
    animSetup(matrix, width);

    setup_handlers();

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

    sockfd = start_udp_server();
    if ( sockfd < 0 )
    {
        fprintf(stderr, "start_udp_server failed: %d\n", sockfd);
	return sockfd;
    }


    // Start the default animation, wait 2 seconds and clear
    activeAnimation = animationIdByName("startupserver");
    if ( activeAnimation < 0 )
    {
	activeAnimation = 0;
	fprintf(stderr, "Error: unable to find startup server animation\n");
    }
    callInitAnimationFunction(activeAnimation);
    animationRender();

    usleep(2000000);

    activeAnimation = 0;
    callInitAnimationFunction(activeAnimation);
    animationRender();

    while (running)
    {
	if ( receive_udp_packet(sockfd, buf, BUFSIZE) > 0 )
	{
		// Find the animation requested and initialize it
		activeAnimation = animationIdByName(buf);

		if ( activeAnimation >= 0 )
		{
			printf("Received animation change request to: %s(%d)\n", buf, activeAnimation);
    			callInitAnimationFunction(activeAnimation);
		}
		else
		{
			fprintf(stderr, "Error: Received unrecognized animation change request: %s\n", buf);
			activeAnimation = 0;
		}
	}	

	wait_for_packet_or_timeout(sockfd, sleepTime);

	if ( sleepTime)	callIterateAnimationFunction(activeAnimation);

	if ( animationRender() < 0 )	break;
    }

    if (clear_on_exit)
    {
    	callInitAnimationFunction(0);
	animationRender();
    }

    ws2811_fini(&ledstring);

    close(sockfd);

    printf ("\n");
    return ret;
}

