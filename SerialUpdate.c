/*-------------------------------------------------------------*/
/* termios structure -  /usr/include/asm-generic/termbits.h    */ 
/* use "man termios" to get more info about  termios structure */
/*-------------------------------------------------------------*/

#include <stdio.h>
#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>  /* UNIX Standard Definitions 	   */ 
#include <errno.h>   /* ERROR Number Definitions           */
#include <math.h>
#include <string.h>

#define CMD_GET_INFORMATION   0
#define CMD_ERASE_MCU_FLASH   5
#define CMD_SET_MCU_ADDRESS   7
#define CMD_PROGRAM_MCU_FLASH 6

typedef struct
{
  unsigned int ulAddr0;
  unsigned int ulAddr1;
  unsigned int ulSize;
  unsigned char cInfo[8];
}stInformation;

unsigned char cFInfo[20] = {0};

unsigned char cRxBuffer[1024] = {0};
int iRxWrite = 0;
int iRxRead = 0;

unsigned char cTxBuffer[4112] = {0};
int iTxWrite = 0;

unsigned char cCommand[64] = {0};




int PushRxData(unsigned char *buf , int counts)
{
  while(counts)
  {
    cRxBuffer[iRxWrite] = *(buf++);
    iRxWrite +=1;
    if (iRxWrite >=1024)
    {
      iRxWrite = 0;
    }
    counts -= 1;
  }
  //printf("r:%d,W:%d\n",iRxRead,iRxWrite );
  return 0;
}
int GetRxData(unsigned char *byte)
{
  if(iRxWrite != iRxRead)
  {
    //printf("GetRxData:0x%X\n",(char)cRxBuffer[iRxRead]);
    *byte = cRxBuffer[iRxRead];
    iRxRead +=1;
    if(iRxRead>=1024) iRxRead = 0;
    return 0;
  }
  else return -1;
}
int OpenCom(char *com)
{
  int fd ;
  //fd = open(com,O_RDWR | O_NDELAY);  /* ttyUSB0 is the FT232 based USB2SERIAL Converter   */
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
  SerialPortSettings.c_cflag &= ~INPCK;

  SerialPortSettings.c_cflag &= ~CSTOPB;   /* CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit */
  SerialPortSettings.c_cflag &= ~CSIZE;	 /* Clears the mask for setting the data size             */
  SerialPortSettings.c_cflag |=  CS8;      /* Set the data bits = 8                                 */

  //SerialPortSettings.c_cflag &= ~CRTSCTS;       /* No Hardware flow Control                         */
  SerialPortSettings.c_cflag |= CREAD | CLOCAL; /* Enable receiver,Ignore Modem Control lines       */ 


  //SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          /* Disable XON/XOFF flow control both i/p and o/p */
  SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Non Cannonical mode                            */
  //SerialPortSettings.c_iflag &= ~(ICRNL |INLCR |IGNCR );
  SerialPortSettings.c_oflag &= ~OPOST;/*No Output Processing*/
  /* Setting Time outs */
  SerialPortSettings.c_cc[VMIN] = 0; /* Read at least 10 characters */
  SerialPortSettings.c_cc[VTIME] = 0; /* Wait indefinetly   */
    
  tcflush(fd, TCIFLUSH);   /* Discards old data in the rx buffer            */

  if((tcsetattr(fd,TCSANOW,&SerialPortSettings)) != 0) /* Set the attributes to the termios structure*/
    printf("\n  ERROR ! in Setting attributes");
  else
    printf("\n  BaudRate = 115200  StopBits = 1  Parity   = none");

    
  //int nn = sprintf(cTxBuffer,"serial open success\n");
  //write(fd,cTxBuffer,nn);
  return fd;
}

int CheckSum(unsigned char*buf , int num)
{
  unsigned short sum = 0;
  unsigned short check = buf[num-2]+buf[num-1]*256;
  for(int i=4; i< num-2; i++ )
  {
    sum += buf[i];
  }
  if (sum == check) return 0;
  else return -1;
}
int DecodeData()
{
  static int iIndex = 0;
  static int iSyncFlag = 0;
  static int iDataLen = 0;
  unsigned char cByte = 0;
  while(0==GetRxData(&cByte))
  {
    printf("Get:0x%x,%d\n",cByte,iIndex);
    if (iSyncFlag || ((iIndex == 0) && (cByte == 0xAA))
                  || ((iIndex == 1) && (cByte == 0xBB))
                  || ((iIndex == 2) && (cByte == 0xFF))
                  || ((iIndex == 3) && (cByte == 0xDD)))
    {
      cCommand[iIndex] = cByte;
      iIndex += 1;
      if (iIndex >= 64) iIndex = 0;
      if (iIndex == 4)
      {
        iSyncFlag = 1;
      }
    }
    else
    {
      iSyncFlag = 0;
      iIndex = 0;
    }

    if((iSyncFlag == 1) && (iIndex == 9))
    {
      iSyncFlag = 2;
      iDataLen = 11 + cCommand[6]+cCommand[7]*256;
    }

    if((iSyncFlag == 2) && (iIndex == iDataLen))
    {
      printf("CheckSum Command\n");
      cCommand[iIndex] = 0;
      if(!CheckSum(cCommand,iIndex))
      {
        printf("Receive Command %d\n",cCommand[8] );
        iSyncFlag = 0;
        iDataLen = 0;
        iIndex = 0;
        return cCommand[8];
      }
      iSyncFlag = 0;
      iDataLen = 0;
      iIndex = 0;
    }
  }
  return -1;
}
int SendCommand(unsigned char myAd , unsigned char desAd , unsigned char cmdID , unsigned char *data ,int count)
{
  unsigned short sum = 0;
  int i =0;
  unsigned short len = count;
  cTxBuffer[0] = 0xAA;
  cTxBuffer[1] = 0xBB;
  cTxBuffer[2] = 0xFF;
  cTxBuffer[3] = 0xDD;
  cTxBuffer[4] = myAd;
  cTxBuffer[5] = desAd;
  cTxBuffer[6] = (len & 0xFF);
  cTxBuffer[7] = (len>>8 &0xFF);
  cTxBuffer[8] = cmdID;
  memcpy(&cTxBuffer[9],data ,count);
  for(i = 4; i< (count+5) ;i++)
  {
    sum += cTxBuffer[i];
  }
  cTxBuffer[len+9] = (sum & 0xff);
  cTxBuffer[len+10] = ((sum>>8)&0xff);
  cTxBuffer[len+11] = 0;
  //write(fdSerial,cTxBuffer,len+10);
  printf("SendCommand\n" );
  return len+10;
}
int main(int argc , char *argv[])
{
	int fdSerial,fdPro,fdNav;
	unsigned char cReadBuff[64] = {0};
	int iReadBytes = 0;
  unsigned char cSrcAd = 0;
  unsigned char cDestAd = 0;

  unsigned char cBufTemp[64] = {0};
	int iCommand = 0;

  stInformation stinfo;
	if(argc != 2) 
	{
    printf("ERROR:Parameter Number incorrectly!\n");
	  printf("eg. NewtonUpdate <serial port address> <firmware path>\n");
	  printf("quit\n");
    return 0;
  }
  	fdSerial = OpenCom(argv[1]);

  	while(1)
  	{
  		iReadBytes = read(fdSerial,&cReadBuff[0],60); // Read the data 
      cReadBuff[iReadBytes] = 0;
//      printf("%s\n", strerror(errno));
  		if(iReadBytes>0)
  		{
        printf("\nRead,%d\n",iReadBytes);
        //printf("%s",cReadBuff);
        PushRxData(cReadBuff,iReadBytes);
      }
      if(iRxWrite != iRxRead)
      {
  			iCommand = DecodeData();
  			switch(iCommand)
  			{
          case 99://Start Send get information cmd
            printf("Cpu %d Ready.\n",cCommand[4]);
            cTxBuffer[0] = '@';
            write(fdSerial,cTxBuffer,1);
            //sleep(2);
            int num = SendCommand(cSrcAd,cDestAd,CMD_GET_INFORMATION,NULL,0);
            write(fdSerial,cTxBuffer,num);
          break;
  				case 0://get information
            memcpy((char*)&stinfo,&cCommand[9],20);
            if((cFInfo[9]!=stinfo.cInfo[1])||(cFInfo[11]!=stinfo.cInfo[3])||(cFInfo[12]==stinfo.cInfo[4]))
            {
              printf("MCU type error\n");
              return 0;
            }

  				break;
  				case 1:
  				break;
  				default:
  				break;
  			}
  		}
      usleep(50000);
  	}
}


