#include"socket.c"
#include<unistd.h>
#include<stdio.h>
#include<sys/param.h>
#include<rpc/types.h>
#include<getopt.h>
#include<strings.h>
#include<time.h>
#include<signal.h>
#include<string.h>
#include<error.h>
#include<pthread.h>

static void usage(void)  
{  
  fprintf(stderr,  
      "webbench [选项参数]... URL\n"       
      "  -f|--force               不等待服务器响应\n"  
      "  -r|--reload              重新请求加载（无缓存）\n"  
      "  -t|--time <sec>          运行时间，单位：秒，默认为30秒\n"  
      "  -p|--proxy <server:port> 使用代理服务器发送请求\n"  
      "  -c|--clients <n>         创建多少个客户端，默认为1个\n"  
      "  -9|--http09              使用http0.9协议来构造请求\n"  
      "  -1|--http10              使用http1.0协议来构造请求\n"  
      "  -2|--http11              使用http1.1协议来构造请求\n"  
      "  --get                    使用GET请求方法\n"  
      "  --head                   使用HEAD请求方法\n"  
      "  --options                使用OPTIONS请求方法\n"  
      "  --trace                  使用TRACE请求方法\n"  
      "  -?|-h|--help             显示帮助信息\n"  
      "  -V|--version             显示版本信息\n"  );  
};  

//http请求方法
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3

//相关参数选项的默认值
int method = METHOD_GET;
int clients = 1;
int force = 0;          //默认需要等待服务器相应
int force_reload = 0;     //默认不重新发送请求
int proxyport = 80;     //默认访问80端口，http国际惯例
char* proxyhost = NULL; //默认无代理服务器，因此初值为空
int benchtime = 30;     //默认模拟请求时间 

pthread_mutex_t mutex;

//所用协议版本
int http = 1;   //0:http0.9 1:http1.0 2:http1.1

//存放目标服务器的网络地址
char host[MAXHOSTNAMELEN];

//存放请求报文的字节流
#define REQUEST_SIZE 2048
char request[REQUEST_SIZE];



//构造长选项与短选项的对应
static const struct option long_options[]=
{
  {"force",no_argument,&force,1},
  {"reload",no_argument,&force_reload,1},
  {"time",required_argument,NULL,'t'},
  {"help",no_argument,NULL,'?'},
  {"http09",no_argument,NULL,9},
  {"http10",no_argument,NULL,1},
  {"http11",no_argument,NULL,2},
  {"get",no_argument,&method,METHOD_GET},
  {"head",no_argument,&method,METHOD_HEAD},
  {"options",no_argument,&method,METHOD_OPTIONS},
  {"trace",no_argument,&method,METHOD_TRACE},
  {"version",no_argument,NULL,'V'},
  {"proxy",required_argument,NULL,'p'},
  {"clients",required_argument,NULL,'c'},
  {NULL,0,NULL,0}
};

int speed = 0;
int failed = 0;
long long bytes = 0;
int timeout = 0;
void build_request(const char* url);
static int bench();
static void alarm_handler(int signal);
void benchcore(const char* host,const int port,const char* req);

int main(int argc,char* argv[])
{
  int opt = 0;
  int options_index = 0;
  char* tmp = NULL;

  //首先进行命令行参数的处理
  //1.没有输入选项
  if(argc == 1)
  {
    usage();
    return 1;
  }

  //2.有输入选项则一个一个解析
  while((opt = getopt_long(argc,argv,"frt:p:c:?V912",long_options,&options_index)) != EOF)
  {
    switch(opt)
    {
      case 'f':
        force = 1;
        break;
      case 'r':
        force_reload = 1;
        break;
      case '9':
        http = 0;
        break;
      case '1':
        http = 1;
        break;
      case '2':
        http = 2;
        break;
      case 'V':
        printf("WebBench 1.5 covered by fh\n");
        exit(0);

      case 't':
        benchtime = atoi(optarg);   //optarg指向选项后的参数
        break;
      case 'c':
        clients = atoi(optarg);     //与上同
        break;
      case 'p':   //使用代理服务器，则设置其代理网络号和端口号，格式：-p server:port
        tmp = strrchr(optarg,':'); //查找':'在optarg中最后出现的位置
        proxyhost = optarg;        //

        if(tmp == NULL)     //说明没有端口号
        {
          break;
        }
        if(tmp == optarg)   //端口号在optarg最开头,说明缺失主机名
        {
          fprintf(stderr,"选项参数错误，代理服务器 %s:缺失主机名",optarg);
          return 2;
        }
        if(tmp == optarg + strlen(optarg)-1)    //':'在optarg末位，说明缺少端口号
        {
          fprintf(stderr,"选项参数错误，代理服务器 %s 缺少端口号",optarg);
          return 2;
        }

        *tmp = '\0';      //将optarg从':'开始截断
        proxyport = atoi(tmp+1);     //把代理服务器端口号设置好
        break;

      case '?':
        usage();
        exit(0);
        break;

      default:
        usage();
        return 2;
        break;
    }
  }

  //选项参数解析完毕后，刚好是读到URL，此时argv[optind]指向URL

  if(optind == argc)    //这样说明没有输入URL
  {
    fprintf(stderr,"缺少URL参数\n");
    usage();
    return 2;
  }

  if(benchtime == 0)
    benchtime = 30;

  fprintf(stderr,"webbench: 一款轻巧的网站测压工具 1.5 covered by fh\nGPL Open Source Software\n");

  //OK,我们解析完命令行后，首先先构造http请求报文
  build_request(argv[optind]);    //参数是URL

  //请求报文构造好了
  //开始测压
  printf("\n测试中：\n");

  switch(method)
  {
    case METHOD_OPTIONS:
      printf("OPTIONS");
      break;

    case METHOD_HEAD:
      printf("HEAD");
      break;

    case METHOD_TRACE:
      printf("TRACE");
      break;

    case METHOD_GET:

    default:
      printf("GET");
      break;
  }

  printf(" %s",argv[optind]);

  switch(http)
  {
    case 0:
      printf("(使用 HTTP/0.9)");
      break;

    case 1:
      printf("(使用 HTTP/1.0)");
      break;

    case 2:
      printf("(使用 HTTP/1.1)");
      break;
  }

  printf("\n");

  printf("%d 个客户端",clients);

  printf(",%d s",benchtime);

  if(force)
    printf(",选择提前关闭连接");

  if(proxyhost != NULL)
    printf(",经由代理服务器 %s:%d   ",proxyhost,proxyport);

  if(force_reload)
    printf(",选择无缓存");

  printf("\n");   

  //真正开始压力测试
  return bench();
}

void build_request(const char* url)
{
  char tmp[10];
  int i = 0;

  bzero(host,MAXHOSTNAMELEN);
  bzero(request,REQUEST_SIZE);

  //缓存和代理都是http1.0后才有的
  //无缓存和代理都要在http1.0以上才能使用
  //因此这里要处理一下，不然可能会出问题
  if(force_reload && proxyhost != NULL && http < 1)
    http = 1;
  //HEAD请求是http1.0后才有
  if(method == METHOD_HEAD && http < 1)
    http = 1;
  //OPTIONS和TRACE都是http1.1才有
  if(method == METHOD_OPTIONS && http < 2)
    http = 2;
  if(method == METHOD_TRACE && http < 2)
    http = 2;

  //开始填写http请求
  //请求行
  //填写请求方法
  switch(method)
  {
    case METHOD_HEAD:
      strcpy(request,"HEAD");
      break;
    case METHOD_OPTIONS:
      strcpy(request,"OPTIONS");
      break;
    case METHOD_TRACE:
      strcpy(request,"TRACE");
      break;
    default:
    case METHOD_GET:
      strcpy(request,"GET");
  }

  strcat(request," ");

  //判断URL的合法性
  //1.URL中没有"://"
  if(strstr(url,"://") == NULL)
  {
    fprintf(stderr,"\n%s:是一个不合法的URL\n",url);
    exit(2);
  }

  //2.URL过长
  if(strlen(url) > 1500)
  {
    fprintf(stderr,"URL 长度过过长\n");
    exit(2);
  }

  //3.没有代理服务器却填写错误
  if(proxyhost == NULL)   //若无代理
  {
    if(strncasecmp("http://",url,7) != 0)  //忽略大小写比较前7位
    {
      fprintf(stderr,"\nurl无法解析，是否需要但没有选择使用代理服务器的选项？\n");
      usage();
      exit(2);
    }
  }

  //定位url中主机名开始的位置
  //比如  http://www.xxx.com/
  i = strstr(url,"://") - url + 3;

  //4.在主机名开始的位置找是否有'/'，若没有则非法
  if(strchr(url + i,'/') == NULL)
  {
    fprintf(stderr,"\nURL非法：主机名没有以'/'结尾\n");
    exit(2);
  }

  //判断完URL合法性后继续填写URL到请求行

  //无代理时
  if(proxyhost == NULL)
  {
    //有端口号时，填写端口号
    if(index(url+i,':') != NULL && index(url,':') < index(url,'/'))
    {
      //设置域名或IP
      strncpy(host,url+i,strchr(url+i,':') - url - i);

      bzero(tmp,10);
      strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/') - index(url+i,':')-1);

      //设置端口号
      proxyport = atoi(tmp);
      //避免写了':'却没写端口号
      if(proxyport == 0)
        proxyport = 80;
    }
    else    //无端口号
    {
      strncpy(host,url+i,strcspn(url+i,"/"));   //找到url+i到第一个”/"之间的字符个数
    }
  }
  else    //有代理服务器就简单了，直接填就行，不用自己处理
  {
    strcat(request,url);
  }

  //填写http协议版本到请求行
  if(http == 1)
    strcat(request," HTTP/1.0");
  if(http == 2)
    strcat(request," HTTP/1.1");

  strcat(request,"\r\n");

  //请求报头

  if(http > 0)
    strcat(request,"User-Agent:WebBench 1.5\r\n");

  //填写域名或IP
  if(proxyhost == NULL && http > 0)
  {
    strcat(request,"Host: ");
    strcat(request,host);
    strcat(request,"\r\n");
  }

  //若选择强制重新加载，则填写无缓存
  if(force_reload && proxyhost != NULL)
  {
    strcat(request,"Pragma: no-cache\r\n");
  }

  //我们目的是构造请求给网站，不需要传输任何内容，当然不必用长连接
  //否则太多的连接维护会造成太大的消耗，大大降低可构造的请求数与客户端数
  //http1.1后是默认keep-alive的
  if(http > 1)
    strcat(request,"Connection: close\r\n");

  //填入空行后就构造完成了
  if(http > 0)
    strcat(request,"\r\n");
}


//线程入口函数
void *ThreadEntry(void* arg)
{  
  //发出请求报文
  benchcore(proxyhost == NULL?host : proxyhost,proxyport,request);

  return NULL;
}

//父进程的作用：创建子进程，读子进程测试到的数据，然后处理
static int bench()
{
  int i = 0;

  //尝试建立连接一次
  i = Socket(proxyhost == NULL?host:proxyhost,proxyport);

  if(i < 0)
  {
    fprintf(stderr,"\n连接服务器失败，中断测试\n");
    return 3;
  }

  close(i);//尝试连接成功了，关闭该连接

  //动态开辟一个数组，用来保存线程ID，让pthread_join函数使用
  pthread_t* arrtid = (pthread_t*)malloc(sizeof(pthread_t)*clients);
  //多线程去测试，建立多少个线程进行连接由参数clients决定
  for(i = 0;i < clients;i++)
  {
    pthread_t tid;
    pthread_create(&tid,NULL,ThreadEntry,NULL);
    arrtid[i] = tid;
    //pthread_detach(tid);
  }

  for(i = 0;i < clients;i++)
  {
    pthread_join(arrtid[i],NULL);
  }
  //sleep(benchtime+1);
  //统计处理结果
  printf("\n请求：%d 成功,%d 失败\n", speed,failed);
  printf("每秒连接的字节数为：%d\n",(int)(bytes/(float)benchtime));
  printf("总连接数为:%d\n",speed+failed);
  printf("每秒完成的http连接数为：%d\n",(speed+failed)/benchtime);
  printf("每分钟完成的http连接数为：%d\n",(int)((speed+failed)/(benchtime/60.0f))); 
  return i;
}

//闹钟信号处理函数
static void alarm_handler(int signal)
{
  timeout = 1;
}
//线程真正的向服务器发出请求报文并以其得到此期间的相关数据
void benchcore(const char* host,const int port,const char* req)
{
  int rlen;
  char buf[1500];
  int s,i;
  struct sigaction sa;

  //安装闹钟信号的处理函数
  sa.sa_handler = alarm_handler;
  sa.sa_flags = 0;
  if(sigaction(SIGALRM,&sa,NULL))
    exit(3);

  //设置闹钟函数
  alarm(benchtime);

  rlen = strlen(req);

nexttry:
  while(1)
  {
    //只有在收到闹钟信号后会使 time = 1
    //即该线程的工作结束了
    pthread_mutex_lock(&mutex);
    if(timeout)
    {
      if(failed > 0)
      {
        failed--;
        printf("failed--%d\n",failed);
        pthread_mutex_unlock(&mutex);
      }
      else 
      {
        pthread_mutex_unlock(&mutex);
      }
      return;
    }
    else 
    {
      pthread_mutex_unlock(&mutex);
    }
    

    //建立到目标网站服务器的tcp连接，发送http请求
    s = Socket(host,port);
    pthread_mutex_lock(&mutex);
    if(s < 0)
    {
      failed++;  //连接失败
      printf("failed++%d\n",failed);
      pthread_mutex_unlock(&mutex); 
      continue;
    }
    else 
    {
       pthread_mutex_unlock(&mutex); 
    }

    //发送请求报文
    pthread_mutex_lock(&mutex); 
    if(rlen != write(s,req,rlen))
    {
      failed++;
      printf("faile++%d\n",failed);
      pthread_mutex_unlock(&mutex);
      close(s);   //写失败了关闭套接字
      continue;
    }
    else 
    {
      pthread_mutex_unlock(&mutex);
    }

    //http0.9的特殊处理
    //因为http0.9是在服务器回复后自动断开连接的，不keep-alive
    //在此可以提前先彻底关闭套接字的写的一半，如果失败了那么肯定是个不正常的状态,
    //如果关闭成功则继续往后，因为可能还有需要接收服务器的恢复内容
    //但是写这一半是一定可以关闭了，作为客户端进程上不需要再写了
    //因此我们主动破坏套接字的写端，但是这不是关闭套接字，关闭还是得close
    //事实上，关闭写端后，服务器没写完的数据也不会再写了，这个就不考虑了
    pthread_mutex_lock(&mutex); 
    if(http == 0)
    {
      if(shutdown(s,1))
      {
        failed++;

        printf("failed++%d\n",failed);
        pthread_mutex_unlock(&mutex); 
        close(s);
        continue;
      }
      else 
      {
        pthread_mutex_unlock(&mutex); 
      }
    }
    else 
    {
      pthread_mutex_unlock(&mutex); 
    }

    //-f没有设置时默认等待服务器的回复
    if(force == 0)
    {
      while(1)
      {
        if(timeout)
          break;
        i = read(s,buf,1500);   //读服务器发回的数据到buf中
        pthread_mutex_lock(&mutex); 
        if(i < 0)
        {
          failed++;   //读失败
          
          printf("failed++%d\n",failed);
          pthread_mutex_unlock(&mutex); 
          close(s);   //失败后一定要关闭套接字，不然失败个数多时会严重浪费资源
          goto nexttry;   //这次失败了那么继续下一次连接，与发出请求
        }
        else
        {
          if(i == 0)  //读完了
          {
            pthread_mutex_unlock(&mutex);
            break;
          }
          else
          {
            bytes += i; //统计服务器回复的字节数
            printf("bytes %d\n",bytes);
            pthread_mutex_unlock(&mutex); 
          } 
        }

      }
    }
    pthread_mutex_lock(&mutex); 
    if(close(s))
    {
      failed++;
      pthread_mutex_unlock(&mutex); 
      continue;
    }
    else 
    {
      pthread_mutex_unlock(&mutex); 
    }
    pthread_mutex_lock(&mutex); 
    speed++;

    printf("speed++----------------------------------%d\n",speed);
    pthread_mutex_unlock(&mutex); 
  }
}

