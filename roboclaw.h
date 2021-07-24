#ifndef ROBOCLAWH
#define ROBOCLAWH
#define UNIX

#define MOTORAADDR	128
#define MOTORBADDR	129
#define MOTORCADDR	130
// Packet serial commands
#define ROBOREADFIRMWAREVERSION	21
#define ROBOREADMAINVOLTAGE		24
#define ROBOREADLOGICVOLTAGE		25
#define ROBOSETMODE				74
#define ROBOFORWARD				0
#define ROBOBACKWARD			1

#include <stdio.h>
#include <string.h>

#ifdef UNIX
#define DEFAULTSERIAL "/dev/ttyS3"
#define msleep(t)	usleep(1000*t)	// Milliseconds sleep
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#endif

uint16_t crc16(uint8_t*, uint16_t);
int init_ROBOCLAW(int, char **);
uint8_t get_RoboFirmware(int, uint8_t, uint8_t*);
float get_RoboLogicVolts(int, uint8_t);
float get_RoboMainVolts(int, uint8_t);
uint8_t set_ROBOMode(int, uint8_t);
uint8_t move(int, uint8_t, uint8_t, uint8_t);

int serialInit(char *);
int serialRecv(int, uint8_t*, int);
int serialSend(int, uint8_t*, int);

#endif
