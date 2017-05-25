#include "sysInclude.h"
#include <iostream>
#include <vector>
#include <bitset>
using namespace std;

extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

typedef unsigned int uint;

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

struct node 
{
  bool isOne;
  uint nextIP;
  node *lson, *rson;
  node()
  {
    isOne = false;
    lson = rson = NULL;
  }
};

node *root;

void stud_Route_Init()
{
  root = new node();
	return;
}

void stud_route_add(stud_route_msg *proute)
{
  node *now = root;
  bitset<32> dest = htonl(proute->dest);
  uint masklen = htonl(proute->masklen);
  uint nextIP = htonl(proute->nexthop);
  cout<<"add IP="<<dest<<endl;
  uint dep = 32;
  while(dep + masklen >= 32)
  {
    if(dep == masklen)
    {
      now->isOne = true;
      now->nextIP = nextIP;
      cout<<"add "<<dep<<" "<<nextIP<<endl;
      break;
    }
    
    if(dest[dep] == 0)
    {
      if(!now->lson)
        now->lson = new node();
      now = now->lson;
    }
    else
    {
      if(!now->rson)
        now->rson = new node();
      now = now->rson;
    }
    dep--;

  }

  return;
}


bool getNextIP(uint destIP, uint &nextIP)
{
  destIP = destIP;
  bitset<32> dest = htonl(destIP);
  cout<<dest[0]<<" "<<dest[1]<<" "<<dest[2]<<" "<<dest[3]<<" "<<dest[4]<<" "<<dest[5]<<" "<<dest[6]<<" "<<dest[7]<<endl;
  cout<<"query "<<dest<<endl;
  uint dep = 32;
  bool hasOne = false;

  node *now = root;
  for(; ; )
  {
    if(now->isOne)
    {
      cout<<"find one!!! "<<dep<<endl;
      nextIP = now->nextIP;
      hasOne = true;
    }

    if(dest[dep] == 0)
    {
      if(!now->lson)
        break;
      now = now->lson;
    }
    else
    {
      if(!now->rson)
        break;
      now = now->rson;
    }
    dep--;
  }
  return hasOne;
}

int stud_fwd_deal(char *pBuffer, int length)
{
  // dest IP
  uint destIP;
  destIP = get32(pBuffer + 16);
  uint localIP;
  localIP = getIpv4Address();
  if(destIP == 0xFFFFFFFF || destIP == localIP)
  {
    fwd_LocalRcv(pBuffer, length);
    return 0;
  }
  // dest IP end

  uint nextIP;
  if(getNextIP(destIP, nextIP))
  {
    // ttl
    uint ttl;
    ttl = get8(pBuffer + 8);
    if(ttl == 0)
    {
      fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
      return 1;
    }
    ttl = ttl - 1;
    pBuffer[8] = setChar(ttl);
    // ttl end

    // check sum
    uint checkSum;
    checkSum = 0;
    int i;
    for(i = 0; i < 20; i+=2)
      if(i != 10)
        checkSum += get16(pBuffer + i);

    while(checkSum > 0xFFFF)
      checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
    checkSum = ((~checkSum) & 0xFFFF);
    pBuffer[10] = setChar(checkSum >> 8);
    pBuffer[11] = setChar(checkSum & 0xFF);
    // check sum end

    fwd_SendtoLower(pBuffer, length, nextIP);
    return 0;
  }
  else
  {
    fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
    return 1;
  }
}

