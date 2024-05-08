#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  // client, server의 listendfd, connfd socket 생성
  int client_listenfd, client_connfd; 
  char hostname[MAXLINE], clientPort[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  printf("%s", user_agent_hdr);

  // './proxy 8080'처럼 2개 argc가 입력되지 않았다면
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  client_listenfd = Open_listenfd(argv[1]); 
  while (1) {
    clientlen = sizeof(clientaddr);
    //Accept = 새로운 소켓 디스크립터가 생성
    client_connfd = Accept(client_listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept  // 연결요청 받기
    //소켓 주소 구조체를 대응되는 호스트와 서비스 이름 스트링으로 변환    
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, clientPort, MAXLINE, 0);
    //proxy 터미널에서 요청이 왔는지 나타내기
    printf("Accepted connection from (%s, %s)\n", hostname, clientPort);
    
    // doit(connfd);   // line:netp:tiny:doit // performing transaction
    rio_t rio;
    char buf[MAXLINE];
    // rio 초기화
    Rio_readinitb(&rio, client_connfd);
    //client에서 들어온 내용을 buf에 담기
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("client에서 온 내용입니다.:\n");
    printf("%s \n", buf);

    Close(client_connfd);  // line:netp:tiny:close // 연결 닫기
  }
  return 0;
}

// void doit(int fd)
// {
//   int is_static;
//   struct stat sbuf;
//   char request_buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
//   char filename[MAXLINE], cgiargs[MAXLINE];
//   rio_t request_rio;

//   // Read request line and headers
//   Rio_readinitb(&request_rio, fd);
//   Rio_readlineb(&request_rio, buf, MAXLINE); //read and parse the request line 55-58
//   printf("Request headers:\n");
//   printf("%s", buf);

//   //method, uri, version 찾기	
//   sscanf(buf, "%s %s %s", method, uri, version);
//   parse_uri(uri, hostname, port, path);

//   //Server에 전송하기 위한 요청 라인의 형식
//   sprintf(request_buf, "%s %s %s\r\n", method, path, "HTTP/1.0");

//   if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  //GET과 HEAD만 받는다. 아닐시 연결을 끊고 다음 연결 요청을 기다림 (HEAD 요청 받기 추가)
//     clienterror(fd, filename, "501", "Not implemented", "Tiny does not implement this method");
//     return;
//   }

//   read_requesthdrs(&request_rio); //tiny server에서 header는 읽기만 하고 무시함

//   //Parse URI
//   is_static = parse_uri(uri, filename, cgiargs); //URI를 filename과 (possibly) empty CGI argument string으로 parse하고, 정적 컨텐츠인지 동적 컨텐츠인지 판단하는 flag설정(1이면 정적, 0이면 동적인듯??)
//   if (stat(filename, &sbuf) < 0) {
//     clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
//     return;
//   }
// }

// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
// {
//   char buf[MAXLINE], body[MAXBUF];

//   // Build the HTTP response body
//   sprintf(body, "<html><title>Tiny Error</title>");
//   sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
//   sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
//   sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
//   sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

//   // Print the HTTP response
//   sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
//   Rio_writen(fd, buf, strlen(buf));
//   sprintf(buf, "Content-type: text/html\r\n");
//   Rio_writen(fd, buf, strlen(buf));
//   sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
//   Rio_writen(fd, buf, strlen(buf));
//   Rio_writen(fd, body, strlen(body));
// }

// int parse_uri(char *uri, char *filename, char *cgiargs)
// {
//   char *ptr;
//   // 현재 디렉토리가 정적 컨텐츠를 위한 home directory고 ./cgi-bin이 동적 컨텐츠를 위한 home directory
//   if (!strstr(uri, "cgi-bin")) { //Static content //"cgi-bin"이라는 문자열을 포함하는 모든 uri는 동적 컨텐츠일 것으로 예측
//       strcpy(cgiargs, ""); //정적 컨텐츠니까 CGI argument string을 비우고. strcpy = cgiargs를 ""로 대체
//       strcpy(filename, "."); // filename을 .으로 변환
//       strcat(filename, uri); // .으로 변환된 filename 뒤에 uri 붙이기
//       if (uri[strlen(uri)-1] == '/') // 만약 uri가 /로 끝나면 뒤에 default filename 붙이기
//         strcat(filename, "home.html"); //default filename = ./home.html
//       return 1;
//   }
//   else { //Dynamic content
//       ptr = index(uri, '?'); //'?'가 위치한 index에 ptr두고,
//       if (ptr){
//         strcpy(cgiargs, ptr+1); // 인자들 추출
//         *ptr ='\0'; // 포인터를 종단문자로 변경
//       }
//       else
//         strcpy(cgiargs, "");
//       strcpy(filename, ".");
//       strcat(filename, uri);
//       return 0;
//   }
// }
