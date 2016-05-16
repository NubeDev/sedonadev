/*	I2C Application test code for Arduino Uno Board (slave address 0x09)
*	Author	: Titusrathinaraj Stalin
*	Date	: 30-04-2016
*	Version : 1.0
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "sedona.h"


int file,value=1;
char filename[40];
char buf[10] = {0};
int addr = 0x09;        // The I2C address of the Arduino I2C slave board
bool read_status_i2c = false;
bool read_status_sensor = false;


//for arm_v5t_le- toolchain
#define I2C_SLAVE		0x0703

i2c_I2cDev_doI2cInit(SedonaVM* vm, Cell* params)
	{
//I2C initializing
	sprintf(filename,"/dev/i2c-1");
	if ((file = open(filename,O_RDWR)) < 0) {
	printf("Titus : Failed to open the bus.\n");
	/* ERROR HANDLING; you can check errno to see what went wrong */
//	exit(1);
	return -1;
		}

	//I2C IOCTL
	if (ioctl(file,I2C_SLAVE,addr) < 0) {
	printf("Titus : Failed to acquire bus access and/or talk to slave.\n");
	/* ERROR HANDLING; you can check errno to see what went wrong */
//	exit(1);
	return -1;
		}

	}

i2c_I2cDev_doI2cExit(SedonaVM* vm, Cell* params)
	{
	printf("Titus : Closed the i2c bus.\n");
	close(file);
	}
	


i2c_I2cDev_doI2cWrite(SedonaVM* vm, Cell* params)
{

buf[0] = params[0].ival;

printf("Titus : write to the i2c bus, value %d\n",buf[0]);

//I2C WRITE
if (write(file,buf,1) != 1) {
/* ERROR HANDLING: i2c transaction failed */
printf("Titus : Failed to write to the i2c bus\n");
//exit(1);
	return -1;
printf("\n\n");
	}

}


i2c_I2cDev_doI2cRead(SedonaVM* vm, Cell* params)
{

	if(read_status_sensor)
	{
//	printf("Titus : I2C READ Enabled for sensor\n");
	//I2C READ
	if (read(file,buf,1) != 1) {
	/* ERROR HANDLING: i2c transaction failed */
	printf("Titus : Failed to read from the i2c bus\n");
//	exit(1);
	return -1;
	printf("\n\n");
		}
	 }

//printf("Titus : read from the i2c bus, value %d\n",buf[0]);
return buf[0];

}

i2c_I2cDev_doI2cSensorReadEnStatus(SedonaVM* vm, Cell* params)
{
	read_status_sensor = params[0].ival;
}

#if 0
i2c_I2cDev_doI2cSensorRead(SedonaVM* vm, Cell* params)
{

	if(read_status_sensor)
	{

	//I2C READ
	if (read(file,buf,1) != 1) {
	/* ERROR HANDLING: i2c transaction failed */
	printf("Titus : Failed to read from the i2c bus\n");
//	exit(1);
	return -1;
	printf("\n\n");
		}
	 }

//printf("Titus : read from the i2c bus, value %d\n",buf[0]);
return buf[0];

}
#endif
