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
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

//mag
#define FILE_MAX_SIZE      (1024 * 1024 * 2) //根据加密程序设定文件最大buffer
//yjy
#define CMD_GET_INFORMATION   0
#define CMD_ERASE_MCU_FLASH   5
#define CMD_SET_MCU_ADDRESS   7
#define CMD_PROGRAM_MCU_FLASH 6
//yjy
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
//mag
unsigned char FILE_ID[4] = {'X','W','Y','D'};

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


//yjy
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

/*****mag*****/
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
    unsigned long ulCRC,i;
    ssize_t nCodeLength;
    char *name_start = NULL;
    char filename[64];
    time_t now;
    struct tm *timenow;

    //检查烧写固件是否存在、可读
    if(access(PathDir ,F_OK | R_OK)<0)
    {
        printf("  Read File Error 01:Cannot Find/read %s:%s\n",PathDir,strerror(errno));
        return -1;
    }

    //打开固件
    if((fd = open(PathDir,O_RDONLY | O_NONBLOCK))<0)
    {
        printf("  Read File Error 02:Cannot Open %s:%s\n",PathDir,strerror(errno));
        return -2;
    }

    //分配内存
    FirmwareSt.ucData = (unsigned char *)malloc(FILE_MAX_SIZE);
    if(FirmwareSt.ucData == NULL)
    {
        free(FirmwareSt.ucData);
        printf("  Read FIle Error 03:Out of Memory\n");
        return -3;
    }
    //读取文件帧头（前19字节）
    if(read(fd,FirmwareSt.ucData,19)==19)
    {
        memcpy(&FirmwareSt,FirmwareSt.ucData,19);
        printf("\n");
        time(&now);
        timenow = localtime(&now);
        printf("File information(%s)\n",PathDir);
        printf("  %s",asctime(timenow));
        //检查帧头
        if(memcmp(FirmwareSt.ucFileID,FILE_ID,4) != 0)
        {
            free(FirmwareSt.ucData);
            printf("  Read File Error 04:Incorrect FileID\n");
            return -4;
        }
        printf("  Company    :StarNeto\n");
        printf("  File Length:%d Bytes\n",FirmwareSt.uFileLen);
        //检查设备码
        if(FirmwareSt.ucProductIndex == 1)
        {
            printf("  Product    :Newton-M Serial\n");
        
        }//之后有更多设备码可以用else if添加到这里
        else
        {
            free(FirmwareSt.ucData);
            printf("  Read File Error 05:Incorrect product)\n");
            return -5;
        }
        //检查索引码与文件名是否一致:0x01 - protocol ; 0x02 - Nav;
        //获取文件名
        name_start = strrchr(PathDir,'/');
        if(NULL==name_start)
            memcpy(filename, PathDir, sizeof(PathDir));
        else
            memcpy(filename, name_start+1, sizeof(name_start+1));
        //文件名大小写转换
        for(i=0;i<sizeof(filename);i++)
            *(filename+i) = tolower(*(filename+i));

        if(FirmwareSt.ucDeviceIndex == 0x01)
        {
            if(!memcmp("protocol",filename,8))
                printf("  Device     :Protocol Device\n");
            else
            {
                free(FirmwareSt.ucData);
                printf("  Read File Error 06-01:File Name and Device Index do not Match(Protocol)\n");
                return -6;
            }
        }
        else if(FirmwareSt.ucDeviceIndex == 0x02)
        {
            if(!memcmp("nav",filename,3))
                printf("  Device     :Nav Device\n");
            else
            {
                free(FirmwareSt.ucData);
                printf("  Read File Error 06-02:File Name and Device Index do not Match(Nav)\n");
                return -6;
            }
        }
        else
        {
            free(FirmwareSt.ucData);
            printf("  Read File Error 07:Read File Error 07:Incorrect DeviceIndex\n");
            return -7;
        }
        printf("\n");
    }
    else
    {
        free(FirmwareSt.ucData);
        printf("  Read File Error 08:Empty File\n");
        return -8;
    }


    //读取文件剩余内容（程序数据+CRC校验）
    nCodeLength = read(fd,FirmwareSt.ucData+19,FILE_MAX_SIZE);
    if(nCodeLength <= 0)
    {
        free(FirmwareSt.ucData);
        printf("  Read File Error 09:File Corrupted:%s\n",strerror(errno));  
        return -9;
    }
    else if(nCodeLength > FILE_MAX_SIZE -19)//帧头结构19字节
    {
        free(FirmwareSt.ucData);
        printf("  Read File Error 10:File is too Large\n");
        return -10;
    }

    //从FirmwareSt.ucData最后四字节中取回CRC校验位
    memcpy(&FirmwareSt.crc,(FirmwareSt.ucData + 19 + nCodeLength -4), 4);

    //计算接收数据CRC
    ulCRC = Cal_CRC32(0, FirmwareSt.ucData, nCodeLength+19-4);
    
    //判断CRC
    if(FirmwareSt.crc != ulCRC)
    {
        printf("  Read File Error 11:CRC Error\n");
        return -11;
    }
    //读取文件成功，返回文件描述符
    return fd;
}

void CloseFirmware(int fd)
{
    close(fd);
}

void FindFirmware(char *FolderDir)
{
    DIR *dir;
    struct dirent *ptr;
    char tempName[512],str[256],ProName[20][256],NavName[20][256];
    unsigned short int i;
    unsigned char ProFileNum=0,NavFileNum=0,status=0;
    //检查文件夹是否正确
    if ((dir=opendir(FolderDir)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }
    printf("\n");
    printf("\n");
    //遍历文件夹，寻找开头为protocol和nav的文件并列出
    while ((ptr=readdir(dir)) != NULL)
    {
        if(ptr->d_type == 8)    //file type
        {
            memcpy(tempName,ptr->d_name,strlen(ptr->d_name));
            for(i=0;i<sizeof(tempName);i++)//字母转小写
                *(tempName+i) = tolower(*(tempName+i));
            if(!memcmp(tempName,"protocol",8))
            {
                printf("  No.%d Pro File in Folder:   %s\n",ProFileNum+1,ptr->d_name);
                strcpy(ProName[ProFileNum],ptr->d_name);
                ProFileNum++;
            }
            else if(!memcmp(tempName,"nav",3))
            {
                printf("  No.%d Nav File in Folder:   %s\n",NavFileNum+1,ptr->d_name);
                strcpy(NavName[NavFileNum],ptr->d_name);
                NavFileNum++;
            }
        }
        else
            continue;
    }
    memset(tempName,0,sizeof(tempName));

    //判断读到多少Protocol开头的文件，如果只有一个直接打开，如果有多个请求输入文件名选择
    if(ProFileNum==1)
    {
        strcpy(tempName,FolderDir);//赋文件夹地址至临时字符串
        strcat(tempName,ProName[0]);//文件名追加至临时字符串后成为文件绝对地址
        if(OpenFirmware(tempName)<0)
            exit(1);//如果获取文件内容错误，OpenFirmware函数会返回相应错误，此处退出即可
    }
    else if(ProFileNum>1)
    {
        printf("Select Pro File to Update(Finish by 'Enter'):");
        if((scanf("%[^\n]",str))<0)
            printf("Input Error\n");
        if(str != NULL)
        {
            for(i=0;i<20;i++)
            {
                if(!strcmp(str,ProName[i]))
                {
                    status=1;
                    strcpy(tempName,FolderDir);
                    strcat(tempName,str);
                    if(OpenFirmware(tempName)<0)
                        exit(1);
                }
            }
            //判断输入的字符是否是正确的文件名
            if(status==1)
                status=0;
            else
            {
                printf("Incorrect Protocol File Name\n");
                exit(1);
            }
        }
        else
        {
            printf("No Protocol File Name Input\n");
            exit(1);
        }
    }
    else
    {
        printf("No Protocol File in Folder\n");
        exit(0);
    }
    memset(str,0,sizeof(str));
    memset(tempName,0,sizeof(tempName));//清空字符串
    getchar();//再读一个字节，防止上次输入的回车等对下次输入造成影响
    fflush(stdin);//清空输入流
    
    //判断Nav文件个数，响应方式与Pro相同
    if(NavFileNum==1)
    {
        strcpy(tempName,FolderDir);
        strcat(tempName,NavName[0]);
        if(OpenFirmware(tempName)<0)
            exit(1);
    }
    else if(NavFileNum>1)
    {
        printf("Select Nav File to Update(Finish by 'Enter'):");
        if((scanf("%[^\n]",str))<0)
            printf("Input Error\n");
        if(str != NULL)
        {
            for(i=0;i<20;i++)
            {
                if(!strcmp(str,NavName[i]))
                {
                    status=1;
                    strcpy(tempName,FolderDir);
                    strcat(tempName,str);
                    if(OpenFirmware(tempName)<0)
                        exit(1);
                }
            }
            //判断输入的字符是否是正确的文件名
            if(status==1)
                status=0;
            else
            {
                printf("Incorrect Nav File Name\n");
                exit(1);
            }
        }
        else
        {
            printf("No Nav File Name Input\n");
            exit(1);
        }
    }
    else
    {
        printf("No Nav File in Folder\n");
        exit(0);
    }
    memset(str,0,sizeof(str));
    memset(tempName,0,sizeof(tempName));//清空字符串
    getchar();//再读一个字节，防止上次输入的回车等对下次输入造成影响
    fflush(stdin);//清空输入流
    //printf("TEST!\n");
    //关闭文件夹
    closedir(dir);
}

int main(int argc , char *argv[])
{
	int fdSerial,fdFirmware;
	unsigned char cReadBuff[64] = {0};
	int iReadBytes = 0;
    unsigned char cSrcAd = 0;
    unsigned char cDestAd = 0;
    unsigned char cBufTemp[64] = {0};
	int iCommand = 0;

    stInformation stinfo;
	if(argc != 3) 
	{
        printf("ERROR:Parameter Number incorrectly!\n");
	    printf("eg. NewtonUpdate <serial port address> <firmware path>\n");
	    printf("quit\n");
        return 0;
    }
  	fdSerial = OpenCom(argv[1]);

    //根据传入参数打开导航、协议固件，并根据返回值判断是否成功
    FindFirmware(argv[2]);
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


