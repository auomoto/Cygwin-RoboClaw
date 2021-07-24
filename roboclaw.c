#include "roboclaw.h"

int main(argc, argv)
int argc;
char *argv[];
{

	uint8_t c, controller, strbuf[100];
	int fd;

	controller = MOTORAADDR;

	printf("RoboClaw 2021-03-05 [%d]\n", argc);
	fd = init_ROBOCLAW(argc, argv);
	printf("fd = %d\n", fd);
	if (fd < 0) {
		exit(1);
	}

	for (;;) {
		printf("cmd: ");
		c = getchar();
		switch(c) {
			case 'f':
				printf("read firmware\r\n");
				get_RoboFirmware(fd, controller, strbuf);
				printf("Firmware version: %s\r", (char*) strbuf);
				break;

			case 'M':
				set_ROBOMode(fd, controller);
				break;

			case 'm':
				move(fd, controller, ROBOFORWARD, 10);
				sleep(2);
				move(fd, controller, ROBOFORWARD, 0);
				sleep(1);
				move(fd, controller, ROBOBACKWARD, 20);
				sleep(3);
				move(fd, controller, ROBOBACKWARD, 0);
				sleep(1);
				move(fd, controller, ROBOFORWARD, 5);
				sleep(2);
				move(fd, controller, ROBOFORWARD, 0);
				sleep(1);
				move(fd, controller, ROBOBACKWARD, 10);
				sleep(3);
				move(fd, controller, ROBOBACKWARD, 0);
				break;

			case 'q':
				printf("quit");
				exit(0);
				break;

			case 'v':
				printf("read voltages\r\n");
				printf("Main voltage = %4.1f\r\n", get_RoboMainVolts(fd, controller));
				printf("Logic voltage = %4.1f\r\n", get_RoboLogicVolts(fd, controller));
				break;

			default:
				break;
		}
		c = getchar();	// throw away the <cr>
	}
}

uint8_t get_RoboFirmware(int fd, uint8_t controller, uint8_t *version)
{

	uint8_t cmd, n, tbuf[52];
	uint16_t crcReceived, crcExpected;

	for (n = 0; n < 52; n++) {
		version[n] = 0;
	}

	cmd = ROBOREADFIRMWAREVERSION;
	tbuf[0] = controller;
	tbuf[1] = cmd;
	serialSend(fd, tbuf, 2);
	serialRecv(fd, version, 50); 
	n = strlen(version);
	crcReceived = version[n+1] << 8 | version[n+2];

	tbuf[0] = controller;
	tbuf[1] = cmd;
	tbuf[2] = '\0';
	strcat(tbuf, version);
	crcExpected = crc16(tbuf, strlen(tbuf)+1);
	if (crcExpected != crcReceived) {
		version[0] = '\0';
		return(0);
	} else {
		return(1);
	}
}


/*---------------------------
	We don't use this connection; don't know why it returns
	an actual number, usually just a bit lower than 2V.
----------------------------*/
float get_RoboLogicVolts(int fd, uint8_t controller)
{

	uint8_t cmd, tbuf[4];
	uint16_t crcReceived, crcExpected;
	int value;

	cmd = ROBOREADLOGICVOLTAGE;
	tbuf[0] = controller;
	tbuf[1] = cmd;
	serialSend(fd, tbuf, 2);
	serialRecv(fd, tbuf, 4);
	value = tbuf[0] << 8 | tbuf[1];

	crcReceived = tbuf[2] << 8 | tbuf[3];
	tbuf[2] = tbuf[0];
	tbuf[3] = tbuf[1];
	tbuf[0] = controller;
	tbuf[1] = cmd;
	crcExpected = crc16(tbuf, 4);
	if (crcExpected != crcReceived) {
		return(-666.0);
	} else {
		return((float) value / 10.0);
	}
}

float get_RoboMainVolts(int fd, uint8_t controller)
{

	uint8_t cmd, tbuf[4];
	uint16_t crcReceived, crcExpected;
	int value;

	cmd = 24;
	tbuf[0] = controller;
	tbuf[1] = cmd;
	serialSend(fd, tbuf, 2);
	serialRecv(fd, tbuf, 4);
	value = tbuf[0] << 8 | tbuf[1];

	crcReceived = tbuf[2] << 8 | tbuf[3];
	tbuf[2] = tbuf[0];
	tbuf[3] = tbuf[1];
	tbuf[0] = controller;
	tbuf[1] = cmd;
	crcExpected = crc16(tbuf, 4);
	if (crcExpected != crcReceived) {
		return(-666.0);
	} else {
		return((float) value / 10.0);
	}

}

int init_ROBOCLAW(int argc, char *argv[])
{

	char serialPort[25];

	if (argc > 1) {
		strcpy(serialPort, argv[1]);
	} else {
		strcpy(serialPort, DEFAULTSERIAL);
	}
	return(serialInit(serialPort));

}

uint8_t move(int fd, uint8_t controller, uint8_t direction, uint8_t speed)
{

	uint8_t cmd, ack, tbuf[5];
	uint16_t crc;

	cmd = direction;
	tbuf[0] = controller;
	tbuf[1] = cmd;
	tbuf[2] = speed;
	crc = crc16(tbuf, 3);
	tbuf[3] = (crc >> 8) & 0xFF;
	tbuf[4] = crc & 0xFF;
	serialSend(fd, tbuf, 5);
	serialRecv(fd, &ack, 1);
	if (ack != 0xFF) {
		return(0);
	} else {
		return(1);
	}
}

int serialInit(char *serialPort)
{

	int fd, flags;
	struct termios tty_attrib;

	fd = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		return(fd);
	}

	flags = fcntl(fd, F_GETFL);		// Retrive access mode and status flags
	flags |= O_NONBLOCK;			// Set non-blocking
	fcntl(fd, F_SETFL, flags);		// Save the flags
	tcgetattr(fd, &tty_attrib);		// Get the current attributes
//	cfsetspeed(&tty_attrib, B9600);		// Set the baud rate
	cfsetspeed(&tty_attrib, B38400);		// Set the baud rate
	cfmakeraw(&tty_attrib);			// Set raw mode
	tcflush(fd, TCIOFLUSH);			// Flush read and write data
	tcsetattr(fd, TCSANOW, &tty_attrib);	// Save new attributes

	return(fd);

}

/*------------------------------------------------------------------------------
	serialRecv(fd, *tbuf, nbytes)

	Reads nbytes data from fd into tbuf.

	Returns the number of bytes actually read, or -1 on error.

------------------------------------------------------------------------------*/

int serialRecv(int fd, uint8_t *tbuf, int nbytes)
{

	int n;

	msleep(10);						// Clear the RoboClaw packet buffer
	n = read(fd, tbuf, nbytes);
	if (n < 0) {
		fprintf(stderr, "serialRecv: read from serial port failed\n");
		tcflush(fd, TCIOFLUSH);		// flush read and write data
		return(-1);
	}

	tcflush(fd, TCIOFLUSH);			// flush read and write data

	return(n);

}

/*------------------------------------------------------------------------------
	serialSend(fd, *data, nbytes)

	Sends nbytes of unsigned 8-bit data to fd.
	It starts with a 10 ms sleep. RoboClaw clears I/O buffers
	if there's no communication from the host after 10 ms.
	If a previous command sent to RoboClaw is garbled, it's
	automatically canceled after 10 ms. See page 60 of the
	RoboClaw user manual Rev 5.6 (2015).

	Returns 0 on success, -1 if the number of bytes sent is not nbytes.

------------------------------------------------------------------------------*/
int serialSend(int fd, uint8_t *data, int nbytes)
{

	int n;

	msleep(10);				// guarantee RoboClaw buffers are clear
	tcflush(fd, TCIOFLUSH);			// flush read and write data
	n = write(fd, data, nbytes);
	if (n != nbytes) {
		fprintf(stderr, "serialSend: write to serial port failed\n");
		return(-1);
	}
	if (tcdrain(fd) != 0) {
		fprintf(stderr, "serialSend: tcdrain error\n");
		return(-1);
	}
	return(0);

}

uint8_t set_ROBOMode(int fd, uint8_t controller)
{

	uint8_t n, ack, cmd, tbuf[7];
	uint16_t crcSent;

	ack = 0;
	cmd = ROBOSETMODE;

	tbuf[0] = controller;
	tbuf[1] = cmd;
	tbuf[2] = 0x00;	// S3 mode default (E-Stop latching; we don't have e-stop)
	tbuf[3] = 0x72;	// S4 mode Home(User)/Limit(Fwd)
	tbuf[4] = 0x00;	// S4 mode disabled
	crcSent = crc16(tbuf, 5);
	tbuf[5] = (uint8_t) (crcSent >> 8);
	tbuf[6] = (uint8_t) (crcSent & 0x00FF);

	serialSend(fd, tbuf, 7);
	serialRecv(fd, &ack, 1);
printf("ack=0x%02X\r\n", ack);
	if (n < 0) {
		return(0);
	} else if (ack != 0xFF) {
		return(0);
	} else {
		return(1);
	}
}

/*------------------------------------------------------------------------------
	crc16(uint8_t *packet, uint16_t nbytes)

	RoboClaw expects a CRC16 word at the end of command data packets.
	See page 59 of the RoboClaw user manual Rev 5.6 (2015). Code example
	is on page 60.

	Returns the CRC16 value as an unsigned 16-bit word.
------------------------------------------------------------------------------*/
uint16_t crc16(uint8_t *packet, uint16_t nBytes)
{

	uint8_t bit;
	uint16_t i, crc = 0;

	for (i = 0; i < nBytes; i++) {
		crc = crc ^ ((uint16_t) packet[i] << 8);
		for (bit = 0; bit < 8; bit++) {
			if (crc & 0x8000) {
				crc = (crc << 1) ^ 0x1021;
			} else {
				crc = crc << 1;
			}
		}
	}

	return (crc);

}

