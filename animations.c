/*
 * animations.c
 *
 * Copyright (c) 2017 Jetty 
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

#include <stdint.h>
#include <stdio.h>
#include "animations.h"



static int		heartDirection = 1;
static int		width=1;
static ws2811_led_t	*matrix;

unsigned long		sleepTime;

void animSetup(ws2811_led_t *ledMatrix, int ledCount)
{
	width  = ledCount;
	matrix = ledMatrix;
}


void animInitClear(int param1, int param2, int param3)
{
	int x;

	for (x = 0; x < width; x++)
		matrix[x] = 0;

	sleepTime = 0U;	// We're done, disable ahnimation
}

void animInitFadeToClear(int param1, int param2, int param3)
{
	// Do nothing because we're using the current leds
}

void animIterateFadeToClear(int param1, int param2, int param3)
{
	int x, allZeros = 1, r, g, b;

	for (x = 0; x < width; x ++ )
	{
		if ( matrix[x] > 0 )
		{
			allZeros = 0;
			r = (matrix[x] & 0xFF0000 ) >> 16;
			g = (matrix[x] & 0x00FF00 ) >> 8;
			b = matrix[x] & 0x0000FF;

			if ( r > 0 )	r--;
			if ( g > 0 )	g--;
			if ( b > 0 )	b--;

			matrix[x] = r << 16 | g << 8 | b;
		}
	}

	if ( allZeros )	sleepTime = 0UL;
}

void animInitOrbit(int param1, int param2, int param3)
{
	int x;

	for (x = 0; x < width; x ++ )
	{
		matrix[x] = param1;
	}

	matrix[0] = param2;
	matrix[1] = param2;
	matrix[2] = param2;
}

void animIterateOrbit(int param1, int param2, int param3)
{
	int x, tmp;
	
	tmp = matrix[width-1];
	for ( x = width-1 ; x > 0; x -- )
	{
		matrix[x] = matrix[x-1];
	}
	matrix[0] = tmp;
}

void animInitHeart(int param1, int param2, int param3)
{
	int x;

	heartDirection = 1;
	for (x = 0; x < width; x ++ )
	{
		matrix[x] = param1;
	}
}

void animIterateHeart(int param1, int param2, int param3)
{
	int x;

	for (x = 0; x < width; x ++ )
	{
		matrix[x] += heartDirection * param3;
	}

	if ( matrix[0] >= param2 ) heartDirection = -1;
	if ( matrix[0] <= param1 ) heartDirection = 1;
}

void animInitFull(int param1, int param2, int param3)
{
	int x;

	for (x = 0; x < width; x ++ )
	{
		matrix[x] = param1;
	}
}



Animation animations[] =
{
	{{"clear", "alexapi_clear",					NULL}, 1,	0,        0,		0,	   &animInitClear,       NULL},		// Clear animation should be the first animation, so it's the default
	{{"fadetoclear", "alexapi_startup", "alexapi_fadetoclear",	NULL}, 128,	0,        0,		0,	   &animInitFadeToClear, &animIterateFadeToClear},
	{{"cyan",	  						NULL}, 1,	0x008888, 0,		0,	   &animInitFull,        NULL},
	{{"red", 	  "alexapi_failure",	 			NULL}, 1,	0x880000, 0,		0,	   &animInitFull,        NULL},
	{{"green",	  "alexapi_success",	 			NULL}, 1,	0x008800, 0,		0,	   &animInitFull,        NULL},
	{{"yellow",	  "startupserver",	 			NULL}, 1,	0x888800, 0,		0,	   &animInitFull,        NULL},
	{{"orbitBlue",	  "alexapi_play", 				NULL}, 15, 	0x000088, 0x006688,	0,	   &animInitOrbit,       &animIterateOrbit},
	{{"orbitRed",	  						NULL}, 15,	0xFF0000, 0xFF0088,	0,	   &animInitOrbit,       &animIterateOrbit},
	{{"orbitGreen",	  "alexapi_processing",	 			NULL}, 15,	0x008800, 0x008866,	0,	   &animInitOrbit,       &animIterateOrbit},
	{{"orbitMagenta", 						NULL}, 15,	0x880088, 0x884488,	0,         &animInitOrbit,       &animIterateOrbit},
	{{"orbitCyan",	  						NULL}, 15,	0x00FFFF, 0x88FFFF,	0,	   &animInitOrbit,       &animIterateOrbit},
	{{"orbitYellow",  						NULL}, 15,	0xFFFF00, 0xFFFF88,	0,	   &animInitOrbit,       &animIterateOrbit},
	{{"heartBlue",   "alexapi_recording",	 			NULL}, 15,	0x000044, 0x000088,	0x000004,  &animInitHeart,       &animIterateHeart},
	{{								NULL}, 0,	0,	  0,		0,	   NULL,		 NULL}
};
