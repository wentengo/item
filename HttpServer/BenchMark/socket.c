#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<sys/time.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdarg.h>


int Socket(const char *host,int clientPort)
{
  int sock;
  unsigned long inaddr;
  struct sockaddr_in ad;
  struct hostent *hp;

  //初始化地址
  memset(&ad,0,sizeof(ad));
  ad.sin_family = AF_INET;
  //ad.sin_addr.s_addr = inet_addr(host);

  //把主机名转化成数字
  inaddr = inet_addr(host);

  if(inaddr != INADDR_NONE)
  {
    memcpy(&ad.sin_addr,&inaddr,sizeof(inaddr));
  }
  else 
  {
    //得到IP地址
    hp = gethostbyname(host);
    if(hp == NULL)
    {
      return -1;
    }
    memcpy(&ad.sin_addr,hp->h_addr,hp->h_length);
  }
  ad.sin_port = htons(clientPort);

  //建立socket
  sock = socket(AF_INET,SOCK_STREAM,0);
  if(sock < 0)
  {
    return sock;
  }

  //建立连接
  if(connect(sock,(struct sockaddr*)&ad,sizeof(ad))<0)
  {
    return -1;
  }
  return sock;
}
