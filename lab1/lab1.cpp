#include <winsock.h>
#include <Windows.h>
#include <process.h>
#include <tchar.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>
#include <set>
using namespace std;

#define test

//transfer adress to another adress
map<string, string> Transfer = 
{
  {"www.bilibili.com", "www.hit.edu.cn" },
  {"jwts.hit.edu.cn", "www.qq.com" },
  {"", ""}
};

//ban web adress list
set<string> BanList = 
{
  "www.sina.cn",
  ""
};

const int MAXSIZE = 102400;  //max length
const int HTTP_PORT = 80; //http port

//Http header
struct HttpHeader{
  char method[4]; // POST or GET，(no CONNECT)
  char url[1024]; // host url
  char host[1024]; // host
  char cookie[1024 * 10]; //cookie
  HttpHeader()
  {
    memset(this,0,sizeof(HttpHeader));
  }
};

struct ProxyParam
{
  SOCKET clientSocket;
  SOCKET serverSocket;
};

bool InitSocket(SOCKET &socket, const int socketPort, sockaddr_in &socketAddr);  //init proxy
void ParseHttpHead(char buffer[], HttpHeader *httpHeader, char sendBuffer[]); //
bool ConnectToServer(SOCKET *serverSocket, char host[]);    //
unsigned int __stdcall ProxyThread(void *proxyParam);   //

//proxy
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//thread pool
//const int ProxyThreadMaxNum = 20;
//HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
//DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};

//deal with banlist or transfer list
void replace(char buffer_c[], const string &oldstr, const string &newstr)
{
  string buffer = string(buffer_c);
  /*
  int st = buffer.find("http://");
  int ed = buffer.find(" HTTP/");
  buffer = buffer.substr(0, st) + "http://" + newstr + "/" + buffer.substr(ed);
  */
  while(buffer.find(oldstr) != string::npos)
  {
    int l = buffer.find(oldstr);
    buffer = buffer.substr(0,l) + newstr + buffer.substr(l+oldstr.length());
  }
  memcpy(buffer_c, buffer.c_str(), buffer.length() + 1);
}

// set Proxy-Connection to Connection
void replaceProxyConnection(char buffer_c[])
{
  printf("[replace ProxyConnection]\n");
  string buffer = string(buffer_c);
  string proxyConnection = "Proxy-Connection: ";
  while( buffer.find(proxyConnection) != string::npos)
  {
    int l = buffer.find(proxyConnection);
    buffer = buffer.substr(0, l) + "Connection: " + buffer.substr(l + proxyConnection.length());
  }
  memcpy(buffer_c, buffer.c_str(), buffer.length() + 1);
  printf("[replace ned]\n");
}

int main(int argc, char* argv[])
{
#ifdef test
  freopen("output.txt","w",stdout);
#endif
  printf("starting...\n");
  if(!InitSocket(ProxyServer, ProxyPort, ProxyServerAddr))
  {
    printf("socket error\n");
    return -1;
  }

  printf("proxy running, port: %d\n", ProxyPort);

  SOCKET acceptSocket = INVALID_SOCKET;
  ProxyParam *lpProxyParam = NULL;
  HANDLE hThread;
  //DWORD dwThreadID;

  //always listening
  while(true)
  {
    acceptSocket = accept(ProxyServer, NULL, NULL);
    lpProxyParam = new ProxyParam;
    if(lpProxyParam == NULL)
    {
      printf("null lpProxyParam");
      continue;
    }

    lpProxyParam->clientSocket = acceptSocket;
    hThread = (HANDLE)_beginthreadex(NULL, 0, &ProxyThread, (void *)lpProxyParam, 0, 0);
    CloseHandle(hThread);
    Sleep(200);
  }

  closesocket(ProxyServer);
  WSACleanup();
  return 0;
}

//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: bool
// Qualifier: init socket
//************************************
bool InitSocket(SOCKET &proxySocket, const int socketPort, sockaddr_in &socketAddr)
{
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
  //version 2.2
  wVersionRequested = MAKEWORD(2, 2);
  err = WSAStartup(wVersionRequested, &wsaData);
  if(err != 0)
  {
    //no winsock.dll
    printf("load winsock error，code: %d\n", WSAGetLastError());
    return FALSE;
  }
  if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
  {
    printf("no correct winsock version\n");
    WSACleanup();
    return FALSE;
  }
  proxySocket = socket(AF_INET, SOCK_STREAM, 0);
  if(INVALID_SOCKET == proxySocket)
  {
    printf("create socket error, code: %d\n", WSAGetLastError());
    return FALSE;
  }

  socketAddr.sin_family = AF_INET;
  socketAddr.sin_port = htons(socketPort);
  socketAddr.sin_addr.S_un.S_addr = INADDR_ANY;
  if(bind(proxySocket, (SOCKADDR*)&socketAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
  {
    printf("bind socket error\n");
    return FALSE;
  }
  if(listen(proxySocket, SOMAXCONN) == SOCKET_ERROR)
  {
    printf("listen port :%d error",socketPort);
    return FALSE;
  }
  return TRUE;
}

//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: thread run 
// Parameter: LPVOID lpParameter
//************************************
unsigned int __stdcall ProxyThread(void *proxyParam)
{
#ifdef test
  printf("[new thread]\n");
#endif
  char Buffer[MAXSIZE];
  char *CacheBuffer;
  memset(Buffer, 0, MAXSIZE);
  //sockaddr_in clientAddr;
  //int length = sizeof(sockaddr_in);
  int recvSize;
  int ret = 0;
  recvSize = recv(((ProxyParam *)proxyParam)->clientSocket, Buffer, MAXSIZE, 0);
  if(recvSize <= 0)
  {
#ifdef test
    printf("[client recv %d]\n", recvSize);
#endif
    goto error;    // receive from client
  }

  HttpHeader *httpHeader;
  httpHeader = new HttpHeader();
  CacheBuffer = new char[recvSize + 1];
  memset(CacheBuffer, 0, recvSize + 1);
  memcpy(CacheBuffer, Buffer, recvSize);

  ParseHttpHead(CacheBuffer, httpHeader, Buffer);
  
  replaceProxyConnection(Buffer);
#ifdef test
  //printf("[SEND]\n[/send]\n");
  printf("[SEND]\n%s\n[/send]\n", Buffer);
#endif
  
  delete CacheBuffer;
  if(!ConnectToServer(&(((ProxyParam *)proxyParam)->serverSocket), httpHeader->host))
    goto error;
  
#ifdef test
  printf("[connect] %s ok![/connect]\n", httpHeader->host);
#endif
  
  // select
  fd_set fdread;
  timeval tv;

  //send the client request to server
  ret = send(((ProxyParam *)proxyParam)->serverSocket, Buffer, recvSize, 0);
  if(ret < 0)
  {
#ifdef test
    int error;
    error = WSAGetLastError();
    printf("[send to server error] %d\n", error);
#endif
    goto error;
  }
  //wait for the buffer from server
  //setsockopt(((ProxyParam *)proxyParam)->serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
  //ioctlsocket(((ProxyParam *)proxyParam)->serverSocket,FIONBIO,(unsigned long *)&ul);
  for(; ; ) 
  { 
    FD_ZERO(&fdread);
    FD_SET(((ProxyParam *)proxyParam)->serverSocket, &fdread);
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    recvSize = select(0, &fdread, NULL, NULL, &tv);
    //recvSize = recv(((ProxyParam *)proxyParam)->serverSocket, Buffer, MAXSIZE, 0);
    if(recvSize < 0)
    {
#ifdef test
      int error;
      error = WSAGetLastError();
      printf("[recv from server error] %d\n", error);
#endif
      goto error;
    }
    else if(recvSize == 0)
    {
#ifdef test
      printf("[timeout]\n");
      break;
#endif
    }
    else
    {
      if(FD_ISSET(((ProxyParam *)proxyParam)->serverSocket, &fdread))
      {
        recvSize = recv(((ProxyParam *)proxyParam)->serverSocket, Buffer, MAXSIZE, 0);
        if(recvSize == 0)
          break;
        send(((ProxyParam *)proxyParam)->clientSocket, Buffer, recvSize, 0);
#ifdef test
        printf("[one time] %d [/one time]\n", recvSize);
        printf("[one recv]\n%s\n[/one recv]\n", Buffer);
#endif
      }
    }
  }

#ifdef test
  printf("[all over]\n");
#endif
  //错误处理
error:
#ifdef test
  printf("[close socket]\n");
#endif
  Sleep(200);
  closesocket(((ProxyParam *)proxyParam)->clientSocket);
  closesocket(((ProxyParam *)proxyParam)->serverSocket);
  delete (ProxyParam *)proxyParam;
  //delete httpHeader;
#ifdef test
  printf("[delete thread]\n");
#endif
  _endthreadex(0);
  return ret;
}

//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public
// Returns: void
// Qualifier: parse http head in TCP
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char buffer[], HttpHeader *httpHeader, char sendBuffer[])
{
  char *p;
  char *ptr;
  const char *delim = "\r\n";
  p = strtok_r(buffer, delim, &ptr);//提取第一行
  if(p[0] == 'G')
  {//GET 方式
    memcpy(httpHeader->method, "GET", 3);
    memcpy(httpHeader->url, &p[4], strlen(p) -13);
  }
  else if(p[0] == 'P')
  {//POST 方式
    memcpy(httpHeader->method, "POST", 4);
    memcpy(httpHeader->url, &p[5], strlen(p) - 14);
  }

#ifdef test
  printf("recv client = [%s]\n", p);
#endif
  p = strtok_r(NULL, delim, &ptr);
  while(p) 
  {
#ifdef test
    printf("recv client = [%s]\n",p);
#endif
    switch(p[0]){
      case 'H'://Host
        memcpy(httpHeader->host, &p[6], strlen(p) - 6);
        break;
      case 'C'://Cookie
        if(strlen(p) > 8)
        {
          char header[8];
          memset(header, 0, sizeof(header));
          memcpy(header, p, 6);
          if(!strcmp(header,"Cookie"))
            memcpy(httpHeader->cookie, &p[8], strlen(p) -8);  
        }
        break;
      default:
        break;
    }
    p = strtok_r(NULL, delim, &ptr);
  }

  if(BanList.find(string(httpHeader->host)) != BanList.end())
  {
    printf("[BAN] %s \n",httpHeader->host);
    memset(httpHeader->host, 0, sizeof(httpHeader->host));
  }
  else if(Transfer.find(string(httpHeader->host)) != Transfer.end())
  {
    printf("[Transfer] %s \n",httpHeader->host);
    string target = Transfer[string(httpHeader->host)];
    const char * target_c = target.c_str();
    replace(sendBuffer, string(httpHeader->host), target);
    memcpy(httpHeader->host, target_c, target.length() + 1);
  }
}

//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public
// Returns: BOOL
// Qualifier: create socket to server, and connect
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
bool ConnectToServer(SOCKET *serverSocket, char host[])
{
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(HTTP_PORT);
  HOSTENT *hostent = gethostbyname(host);
  if(!hostent)
    return FALSE;
  in_addr Inaddr=* ((in_addr*) *hostent->h_addr_list);
  serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
  *serverSocket = socket(AF_INET,SOCK_STREAM, 0);
  if(*serverSocket == INVALID_SOCKET)
    return FALSE;
  if(connect(*serverSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
  {
    closesocket(*serverSocket);
    return FALSE;
  }
  return TRUE;
}
