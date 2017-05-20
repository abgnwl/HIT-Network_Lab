#include "sysInclude.h"
#include <stdio.h>
#include <string.h>
typedef unsigned int uint;

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

uint get8(char *buffer)
{
  return (uint)buffer[0] & 0xFF;
}

uint get16(char *buffer)
{
  return (get8(buffer + 0) << 8) + get8(buffer + 1);
}

uint get32(char *buffer)
{
  return (get16(buffer + 0) << 16) + get16(buffer + 2);
}

char setChar(uint i)
{
  return (unsigned char)(i&0xFF);
}

int stud_ip_recv(char *pBuffer,unsigned short length)
{
  int errorType;
  errorType = 0;

  // version
  uint version;
  version = get8(pBuffer + 0) >> 4;
  if(version != 4)
  {
    //printf("[Error: version]");
    errorType = STUD_IP_TEST_VERSION_ERROR;
    goto error;
  }
  // version end

  // ihl
  uint ihl;
  ihl = get8(pBuffer + 0) & 0xF;
  ihl <<= 2;
  if(ihl < 20)
  {
    //printf("[Error: ihl]");
    errorType = STUD_IP_TEST_HEADLEN_ERROR;
    goto error;
  }
  // ihl end

  // ttl
  uint ttl;
  ttl = get8(pBuffer + 8);
  if(ttl == 0)
  {
    //printf("[Error: ttl]");
    errorType = STUD_IP_TEST_TTL_ERROR;
    goto error;
  }
  // ttl end

  // checkSum
  uint sum;
  sum = 0;

  for(int i = 0; i < ihl; i += 2)
    sum += get16(pBuffer + i);

  while(sum > 0xFFFF)
    sum = (sum >> 16) + (sum & 0xFFFF);
  if((~sum & 0xFFFF) != 0)
  {
    //printf("[Error: checkSum] %u \n",sum);
    errorType = STUD_IP_TEST_CHECKSUM_ERROR;
    goto error;
  }
  // checkSum end

  // dest IP
  uint destIP;
  destIP = get32(pBuffer + 16);
  uint localIP;
  localIP = getIpv4Address();
  if(destIP != 0xFFFFFFFF && destIP != localIP)
  {
    //printf("[Error: destIP]");
    errorType = STUD_IP_TEST_DESTINATION_ERROR;
    goto error;
  }
  // dest IP end


  // Up send
  //printf("[now send] %u %u %u\n",length, ihl, length - ihl);
  ip_SendtoUp(pBuffer + ihl, length - ihl);
  return 0;
  // Up send end

  // deal error
error:
  ip_DiscardPkt(pBuffer, errorType);
  return 1;

}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
    unsigned int dstAddr,byte protocol,byte ttl)
{

  uint ipLen = ((uint)len & 0xFF) + 20;
  char *ipBuffer = new char[ipLen];
  memset(ipBuffer, 0, ipLen);
  memcpy(ipBuffer + 20, pBuffer, len);

  ipBuffer[0] = 0x45;
  ipBuffer[1] = 0;
  ipBuffer[2] = ipLen >> 8;
  ipBuffer[3] = ipLen & 0xFF;

  uint id;
  id = rand();
  //id = 0;
  ipBuffer[4] = id >> 8;
  ipBuffer[5] = id & 0xFF;
  ipBuffer[6] = 0;
  ipBuffer[7] = 0;

  ipBuffer[8] = ttl;
  ipBuffer[9] = protocol;

  ipBuffer[12] = (srcAddr & 0xFFFFFFFF) >> 24;
  ipBuffer[13] = (srcAddr & 0x00FFFFFF) >> 16;
  ipBuffer[14] = (srcAddr & 0x0000FFFF) >> 8;
  ipBuffer[15] = (srcAddr & 0x000000FF) >> 0;

  ipBuffer[16] = (dstAddr & 0xFFFFFFFF) >> 24;
  ipBuffer[17] = (dstAddr & 0x00FFFFFF) >> 16;
  ipBuffer[18] = (dstAddr & 0x0000FFFF) >> 8;
  ipBuffer[19] = (dstAddr & 0x000000FF) >> 0;

  uint checkSum;
  checkSum = 0;
  int i;
  for(i = 0; i < 20; i+=2)
    checkSum += get16(ipBuffer + i);

  while(checkSum > 0xFFFF)
    checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
  checkSum = ((~checkSum) & 0xFFFF);
  ipBuffer[10] = checkSum >> 8;
  ipBuffer[11] = checkSum & 0xFF;

  ip_SendtoLower(ipBuffer, ipLen);

  return 0;
}
