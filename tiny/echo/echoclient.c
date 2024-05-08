#include "csapp.h"

int main(int argc, char **argv)
{
  int clientfd;
  char *host, *port, buf[MAXLINE];
  rio_t rio;

  if (argc != 3){
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    exit(0);
  }
  host = argv[1];
  port = argv[2];

  clientfd = Open_clientfd(host, port);
  Rio_readinitb(&rio, clientfd);

  while (Fgets(buf, MAXLINE, stdin) != NULL) {
    Rio_writen(clientfd, buf, strlen(buf)); // 1. 클라이언트가 버퍼에 쓰고 서버로 전송
    Rio_readlineb(&rio, buf, MAXLINE); // 4. 클라이언트가 소켓 내 버퍼(?)(데이터?)에 쓰인 내용을 읽어와서 버퍼에 저장
    Fputs(buf, stdout);
  }
  Close(clientfd);
  exit(0);
}