#include <string>
#include <cstring>
#include <cstdio>
using namespace std;

struct Header
{
  char Buffer[1032];   // 8 byte header, 65527 byte body

  void setType(const string &str)
  {
    unsigned int len = str.length();
    if (len >= 4)
      len = 4;
    else
    {
      for (unsigned int i = len + 1; i < 4; i++)
        Buffer[i] = 0;
    }

    for (unsigned int i = 0; i < len; i++) 
      Buffer[i] = str[i];
  }

  void setSeq(int seq)
  {
    Buffer[4] = ((unsigned int)seq / 256) & 0xFF;
    Buffer[5] = ((unsigned int)seq % 256) & 0xFF;
  }
  
  unsigned int getSeq()
  {
    return ((unsigned int)Buffer[4] & 0xFF) * 256 + ((unsigned int)Buffer[5] & 0xFF);
  }

  void setLength(int length)
  {
    printf("[set length] = %d\n", length);
    Buffer[6] = ((unsigned int)length / 256) & 0xFF;
    Buffer[7] = ((unsigned int)length % 256) & 0xFF;
    printf("[up=%u down=%u]\n", (unsigned int)Buffer[6]&0xFF, (unsigned int)Buffer[7]&0xFF);
    printf("[get length] = %u\n", (((unsigned int)Buffer[6]&0xFF )<<8) + ((unsigned int)Buffer[7]&0xFF));
  }

  unsigned int getLength()
  {
    return (((unsigned int)Buffer[6]&0xFF )<<8) + ((unsigned int)Buffer[7]&0xFF);
  }

  void show()
  {
    unsigned int length = getLength();

    printf("[head] %c%c%c%c seq=%u  length=%u\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3], getSeq(), length);
  }
};


    

