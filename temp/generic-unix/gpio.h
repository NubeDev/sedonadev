//RasPi
#ifndef GPIO_H
#define GPIO_H



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	// I/O access
	volatile unsigned *gpio;

//	static volatile int setup_en = 0;
	

	
	extern int bo2gpio[MAX_BINARY_OUTPUTS];
	
	
//	#define BCM2708_PERI_BASE        0x20000000
//Titus : rPi2
	#define BCM2708_PERI_BASE        0x3F000000

	#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
	
	
	#define PAGE_SIZE (4*1024)
	#define BLOCK_SIZE (4*1024)
	
	// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
	#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
	#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
	#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
	
	#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
	#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0
	
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
