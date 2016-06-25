//
// Copyright 2013 Tridium, Inc. All Rights Reserved.
//
// History:
//   18 Jan 13 Elizabeth McKenney  Adapted from jenDigitalIo
//                                 Heavy plagiarizing from elinux.org/RPi_Low-level_peripherals
//
//   28 March 2016 Gilberto Villani Brito Adapted to work in RPi2 (gilbertovb@gmail.com)
//

#include "sedona.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>

//#include "gpio.h"

// Uncomment this to print diagnostics
//#define SHOW_DEBUG


#define DIO_CTRL_MASK(which) ( 1 << which ) 

#define MIN_DIO_NUM 0
#define MAX_DIO_NUM 27

//
// For the Raspberry Pi, use only GPIOs available on P2 and 3
//
#define NUM_P1_GPIOS 26
int allowedGpios[NUM_P1_GPIOS] = { 2, 3, 4, 7, 8, 9, 10, 11, 14, 15, 17, 18, 22, 23, 24, 25, 27,5,6,13 }; //Titus : added some more GPIOs support for rPi2 & rPi3


// Flag to indicate whether one-time init has occurred since SVM started
bool bIsGpioInitialized = FALSE;


/*****************************************************************************/
//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 01-01-2013


// Access from ARM Running Linux

#define BCM2708_PERI_BASE        0x3F000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
char *gpio_map;

// I/O access
volatile unsigned int *gpio = NULL;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

void setup_io();
void setGpio(int which);
void clrGpio(int which);

/*****************************************************************************/



//
// isValidGpio - Allow only certain GPIOs, defined by allowedGpios[] array
//
bool isValidGpio(int which)
{
  int g;
  for (g=0; g<NUM_P1_GPIOS; g++)
    if (which == allowedGpios[g]) return TRUE;
  return FALSE;
}



// int Dio.doInit()
Cell rpiDigitalIo_Dio_doInit(SedonaVM* vm, Cell* params)
{
  int whichDio = params[0].ival;
  int bDir     = params[1].ival;

#ifdef SHOW_DEBUG
  printf("native:Dio.doInit( %d, %s )\n", whichDio, (bDir?"in":"out"));  
#endif

  //
  // Do one-time gpio setup if not already done
  //
  if (!bIsGpioInitialized)
  {
    setup_io();

    bIsGpioInitialized = TRUE;
  }


  //
  // Check for invalid DIO
  //
  if ( !isValidGpio(whichDio) )
    return negOneCell;


  //
  // Configure DIO direction:
  //   If bDir==0 then configure DIO for output,
  //   if bDir==1 then configure DIO for input;
  //   o/w do nothing.
  //
  if (bDir==0)
  {
    INP_GPIO(whichDio); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(whichDio);
  }
  else if (bDir==1)
  {
    INP_GPIO(whichDio); 
  }


  return zeroCell;
}




// int  Dio.doRead()
Cell rpiDigitalIo_Dio_doRead(SedonaVM* vm, Cell* params)
{
  int whichDio = params[0].ival;


#ifdef SHOW_DEBUG
  printf("native:Dio.doRead( %d )\n", whichDio); 
#endif

  //
  // Do one-time gpio setup if not already done
  //
  if (!bIsGpioInitialized)
  {
    setup_io();

    bIsGpioInitialized = TRUE;
  }

  // 
  // Check for invalid DIO
  //
  if ( !isValidGpio(whichDio) )
    return negOneCell;


  // Assume DIO is set up for input 

  int rdVal = readGpio() & DIO_CTRL_MASK(whichDio); 
  if (rdVal==0)
    return zeroCell;
  else
    return oneCell;
}



// int  Dio.doWrite(bool bValue)
Cell rpiDigitalIo_Dio_doWrite(SedonaVM* vm, Cell* params)
{
  int whichDio = params[0].ival;
  int bValue   = params[1].ival;


#ifdef SHOW_DEBUG
  printf("native:Dio.doWrite( %d, %s )\n", whichDio, (bValue?"true":"false")); 
#endif


  //
  // Do one-time gpio setup if not already done
  //
  if (!bIsGpioInitialized)
  {
    setup_io();
    bIsGpioInitialized = TRUE;
  }

  //
  // Check for invalid DIO
  //
  if ( !isValidGpio(whichDio) )
    return negOneCell;


  // Assume DIO is set up for output 

  // If bValue==0 or 1 then write value to DIO;
  // o/w arg is "null" - do nothing.
  if (bValue==0)
    clrGpio(whichDio);
  else if (bValue==1)
    setGpio(whichDio);

  return zeroCell;
}




//
// setup_io() - copied verbatim from web page
//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem (need to run as root?)\n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = (char *)mmap(
      NULL,                     // Any adddress in our space will do
      BLOCK_SIZE,               // Map length
      PROT_READ|PROT_WRITE,     // Enable reading & writting to mapped memory
      MAP_SHARED,               // Shared with other processes
      mem_fd,                   // File to map
      GPIO_BASE                 // Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   //if ((long)gpio_map < 0) {
   if (gpio_map == MAP_FAILED) {
      perror("setup_io failed in mmap()");
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned int *)gpio_map;

   printf("  GPIO map set up at address %p\n", gpio);

} // setup_io


// 
// Read a GPIO
//    Assume which is valid GPIO
//
int readGpio()
{
  // just guessing address, based on BCM2835 datasheet
  uint32_t gpLevels = *(volatile unsigned int *)(gpio+13);    

#ifdef SHOW_DEBUG
  printf("  Read value from %p [gpio=%p]: %x\n", (gpio+13), gpio, gpLevels);
#endif

  return gpLevels;
}



// 
// Set a GPIO
//    Assume which is valid GPIO
//
void setGpio(int which)
{
#ifdef SHOW_DEBUG
  printf("  About to write value to %p [gpio=%p]\n", (gpio+7), gpio);
#endif

  *(gpio+7) = DIO_CTRL_MASK(which);    // sets bits that are 1, ignores bits that are 0
}


// 
// Clear a GPIO
//    Assume which is valid GPIO
//
void clrGpio(int which)
{
#ifdef SHOW_DEBUG
  printf("  About to write value to %p [gpio=%p]\n", (gpio+10), gpio);
#endif

  *(gpio+10) = DIO_CTRL_MASK(which);   // clears bits that are 1, ignores bits that are 0
}


