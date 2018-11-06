        /*====================================================================================================*/
        /* Serial Port Programming in C (Serial Port Write)                                                   */
	/* Non Cannonical mode                                                                                */
        /*====================================================================================================*/
	
	/*-------------------------------------------------------------*/
    	/* termios structure -  /usr/include/asm-generic/termbits.h    */ 
	/* use "man termios" to get more info about  termios structure */
	/*-------------------------------------------------------------*/

    	#include <stdio.h>
    	#include <fcntl.h>   /* File Control Definitions           */
    	#include <termios.h> /* POSIX Terminal Control Definitions */
    	#include <unistd.h>  /* UNIX Standard Definitions 	   */ 
    	#include <errno.h>   /* ERROR Number Definitions           */

    	void OpenCom(int fd ,char *com)
    	{
        //	int fd;/*File Descriptor*/
		
		/*------------------------------- Opening the Serial Port -------------------------------*/

		/* Change /dev/ttyUSB0 to the one corresponding to your system */

        	fd = open(com,O_RDWR | O_NOCTTY | O_NDELAY);	/* ttyUSB0 is the FT232 based USB2SERIAL Converter   */
			   						/* O_RDWR Read/Write access to serial port           */
									/* O_NOCTTY - No terminal will control the process   */
									/* O_NDELAY -Non Blocking Mode,Does not care about-  */
									/* -the status of DCD line,Open() returns immediatly */                                        
									
        	if(fd == -1)						/* Error Checking */
            	   printf("\n  Error! in Opening ttyS0  ");
        	else
            	   printf("\n  ttyS0 Opened Successfully ");

	
		/*---------- Setting the Attributes of the serial port using termios structure --------- */
		
		struct termios SerialPortSettings;	/* Create the structure                          */

		tcgetattr(fd, &SerialPortSettings);	/* Get the current attributes of the Serial port */

		cfsetispeed(&SerialPortSettings,B115200); /* Set Read  Speed as 9600                       */
		cfsetospeed(&SerialPortSettings,B115200); /* Set Write Speed as 9600                       */

		SerialPortSettings.c_cflag &= ~PARENB;   /* Disables the Parity Enable bit(PARENB),So No Parity   */
		SerialPortSettings.c_cflag &= ~CSTOPB;   /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
		SerialPortSettings.c_cflag &= ~CSIZE;	 /* Clears the mask for setting the data size             */
		SerialPortSettings.c_cflag |=  CS8;      /* Set the data bits = 8                                 */
	
		SerialPortSettings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
		SerialPortSettings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */ 
		
		
		SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
		SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode                            */

		SerialPortSettings.c_oflag &= ~OPOST;/*No Output Processing*/

		if((tcsetattr(fd,TCSANOW,&SerialPortSettings)) != 0) /* Set the attributes to the termios structure*/
		    printf("\n  ERROR ! in Setting attributes");
		else
                    printf("\n  BaudRate = 115200 \n  StopBits = 1 \n  Parity   = none");
			
	        /*------------------------------- Write data to serial port -----------------------------*/

//		char write_buffer[] = "A";	/* Buffer containing characters to write into port	     */	
//		int  bytes_written  = 0;  	/* Value for storing the number of bytes written to the port */ 

//		bytes_written = write(fd,write_buffer,sizeof(write_buffer));/* use write() to send data to port                                            */
									     /* "fd"                   - file descriptor pointing to the opened serial port */
									     /*	"write_buffer"         - address of the buffer containing data	            */
									     /* "sizeof(write_buffer)" - No of bytes to write                               */	
//		printf("\n  %s written to ttyUSB0",write_buffer);
//		printf("\n  %d Bytes written to ttyS0", bytes_written);
//		printf("\n +----------------------------------+\n\n");

//		close(fd);/* Close the Serial port */

    	}


int main(int argc , char *argv[])
{
	int fd;
	char cWriteBuff[4112] = {0};
	int iWriteBytes = 0;
	char cReadBuff[64] = {0};
//	char cCommandBuff[64] = {0};
	int iReadBytes = 0;

	int iCommand = 0;
	if(argc != 2) 
	{
        printf("ERROR:Parameter Number incorrectly!");
	  	printf("eg. NewtonUpdate <serial port address> <firmware path>\n");
	  	printf("quit\n");
    	return 0;
  	}
  	OpenCom(fd,argv[0]);

  	while(1)
  	{
  		iReadBytes = read(fd,&cReadBuff,60); /* Read the data */
  		if(iReadBytes)
  		{
  			iCommand = DecodeData(cReadBuff , iReadBytes);
  			switch(iCommand)
  			{
  				case 0:
  				break;
  				case 1:
  				break;
  				default:
  				break;
  			}
  		}

  		if(iWriteBytes)
  		{
  			bytes_written = write(fd,write_buffer,sizeof(write_buffer));/* use write() to send data to port  */
  		}
  	}
}


