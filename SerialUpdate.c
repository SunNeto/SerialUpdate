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
#include <time.h>

#define FILE_MAX_SIZE      (1024 * 1024 * 2) //根据加密程序设定文件最大buffer

unsigned char FILE_ID[4] = {'X','W','Y','D'};

int OpenCom(int fd ,char *com)
{
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
    
    return fd;
}

typedef struct 
{
    unsigned char  ucFileID[4];//ID帧头
    unsigned int   uFileLen;//文件长度
    unsigned char  ucEncryptMed;//加密算法
    unsigned char  ucProductIndex;//设备码
    unsigned char  ucDeviceIndex;//索引码（0x01:protocol  0x02:nav）
    unsigned short usHardVer;//硬件版本
    unsigned short usSoftVer;//软件版本
    unsigned int   uExtraCode;//附加码
    unsigned char  *ucData;//程序数据
    unsigned long  crc;//CRC32
}Firmware;
Firmware FirmwareSt;

//CRC32 Table

static const unsigned long crc_table[256] = {

  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

//CRC32 Function

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

unsigned long Cal_CRC32(unsigned long crc, unsigned char *buf, unsigned long len)
{
    if (buf == NULL) return 0L;
    crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
      DO8(buf);
      len -= 8;
    }
    if (len) do {
      DO1(buf);
    } while (--len);
    return crc ^ 0xffffffffL;
}

int OpenFirmware(char *PathDir)
{
    int fd;
    unsigned long ulCRC;
    ssize_t nCodeLength;
    time_t now;
    struct tm *timenow;

    //检查烧写固件是否存在、可读
    if(access(PathDir ,F_OK | R_OK)<0)
    {
        printf("Cannot Find/read %s:%s\n",PathDir,strerror(errno));
        return -1;
    }

    //打开固件
    if((fd = open(PathDir,O_RDONLY | O_NONBLOCK))<0)
    {
        printf("Open %s Error:%s\n",PathDir,strerror(errno));
        return -2;
    }

    //分配内存
    FirmwareSt.ucData = (unsigned char *)malloc(FILE_MAX_SIZE);
    if(FirmwareSt.ucData == NULL)
    {
        free(FirmwareSt.ucData);
        printf("Out of Memory\n");
        return -3;
    }
    int i;
    //读取文件帧头（前19字节）
    if(read(fd,FirmwareSt.ucData,19)==19)
    {
        memcpy(&FirmwareSt,FirmwareSt.ucData,19);
        printf("\n");
        printf("\n");
        time(&now);
        timenow = localtime(&now);
        printf("%s",asctime(timenow));
        printf("文件信息(%s)\n",PathDir);
        printf("帧头    ：%c%c%c%c\n",FirmwareSt.ucFileID[0],FirmwareSt.ucFileID[1],FirmwareSt.ucFileID[2],FirmwareSt.ucFileID[3]);
        printf("文件长度：%d Bytes\n",FirmwareSt.uFileLen);
        printf("加密算法：%d\n",FirmwareSt.ucEncryptMed);
        printf("设备码  ：%d\n",FirmwareSt.ucProductIndex);
        printf("索引码  ：%d\n",FirmwareSt.ucDeviceIndex);
        printf("硬件版本：%d\n",FirmwareSt.usHardVer);
        printf("软件版本：%d\n",FirmwareSt.usSoftVer);
        printf("附加码  ：%x\n",FirmwareSt.uExtraCode);
        printf("\n");
    }
    else
    {
        free(FirmwareSt.ucData);
        printf("Empty File\n");
        return -4;
    }

    //检查帧头
    if(memcmp(FirmwareSt.ucFileID,FILE_ID,4) != 0)
    {
        free(FirmwareSt.ucData);
        printf("Error FileID\n");
        return -5;
    }

    //读取文件剩余内容（程序数据+CRC校验）
    nCodeLength = read(fd,FirmwareSt.ucData+19,FILE_MAX_SIZE);
    if(nCodeLength <= 0)
    {
        free(FirmwareSt.ucData);
        printf("Read File Error:%s\n",strerror(errno));  
        return -6;
    }
    else if(nCodeLength > FILE_MAX_SIZE -19)//帧头结构19字节
    {
        free(FirmwareSt.ucData);
        printf("File is too Large\n");
        return -7;
    }

    //从FirmwareSt.ucData最后四字节中取回CRC校验位
    memcpy(&FirmwareSt.crc,(FirmwareSt.ucData + 19 + nCodeLength -4), 4);

    //计算接收数据CRC
    ulCRC = Cal_CRC32(0, FirmwareSt.ucData, nCodeLength+19-4);
    
    //判断CRC，返回CRC校验错误或文件描述符
    if(FirmwareSt.crc != ulCRC)
    {
        printf("File CRC Error\n");
        return -8;
    }
    else
    {
        printf("Read File Success!\n");
        return fd;
    }
}

void CloseFirmware(int fd)
{
    close(fd);
}

int main(int argc , char *argv[])
{
	int fdSerial,fdFirmware;
    unsigned char ucStatus;
	char cWriteBuff[4112] = {0};
	int iWriteBytes = 0;
	char cReadBuff[64] = {0};
	int iReadBytes = 0;

	int iCommand = 0;
	if(argc != 3) 
	{
        printf("ERROR:Parameter Number incorrectly!");
	  	printf("eg. NewtonUpdate <serial port address> <firmware path>\n");
	  	printf("quit\n");
    	return 0;
  	}
  	//OpenCom(fdSerial,argv[0]);

    //根据传入参数打开导航、协议固件，并根据返回值判断是否成功
    fdFirmware = OpenFirmware(argv[2]);
#if 0
  	while(1)
  	{
  		iReadBytes = read(fdSerial,&cReadBuff,60); /* Read the data */
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
  			iWriteBytes = write(fdPro,cWriteBuff,sizeof(cWriteBuff));/* use write() to send data to port  */
  		}
  	}
#endif
}


