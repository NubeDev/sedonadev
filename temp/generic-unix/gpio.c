//
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//


// Access from ARM Running Linux


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
#include "gpio.h"



//int main(int argc, char **argv)
//{ int g,rep;

  // Set up gpi pointer for direct register access
//  setup_io();

  // Switch GPIO 7..11 to output mode

 /************************************************************************\
  * You are about to change the GPIO settings of your computer.          *
  * Mess this up and it will stop working!                               *
  * It might be a good idea to 'sync' before running this program        *
  * so at least you still have your code changes written to the SD-card! *
 \************************************************************************/

  // Set GPIO pins 7-11 to output
//  for (g=7; g<=11; g++)
// {
//    INP_GPIO(g); // must use INP_GPIO before we can use OUT_GPIO
//    OUT_GPIO(g);
//  }

//  for (rep=0; rep<10; rep++)
//  {
//     for (g=7; g<=11; g++)
//     {
//       GPIO_SET = 1<<g;
//       sleep(1);
//     }
//     for (g=7; g<=11; g++)
//     {
//       GPIO_CLR = 1<<g;
//       sleep(1);
//     }
//  }

//  return 0;

//} // main


//
// Set up a memory regions to access GPIO
//

