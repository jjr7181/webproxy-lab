/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *method, char *filename, int filesize); // HEAD 요청을 위해서 Method도 받도록 수정
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *method, char *filename, char *cgiargs); // HEAD 요청을 위해서 Method도 전달
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); 
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, // 연결요청 받기
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit // performing transaction
    Close(connfd);  // line:netp:tiny:close // 연결 닫기
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t request_rio;

  // Read request line and headers
  Rio_readinitb(&request_rio, fd);
  Rio_readlineb(&request_rio, request_buf, MAXLINE); //read and parse the request line 55-58
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  //GET과 HEAD만 받는다. 아닐시 연결을 끊고 다음 연결 요청을 기다림 (HEAD 요청 받기 추가)
    clienterror(fd, filename, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio); //tiny server에서 header는 읽기만 하고 무시함

  //Parse URI from GET request
  is_static = parse_uri(uri, filename, cgiargs); //URI를 filename과 (possibly) empty CGI argument string으로 parse하고, 정적 컨텐츠인지 동적 컨텐츠인지 판단하는 flag설정(1이면 정적, 0이면 동적인듯??)
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) { //Serve static content
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR &  sbuf.st_mode)){ // !S_ISREG = 일반 파일이 아니고, !S_IRUSE = 읽기 권한이 없을 때
        clienterror(fd, filename, "402", "Forbidden", "Tiny couldn't read the file");
        return;
    }
    serve_static(fd, method, filename, sbuf.st_size);
  }
  else { //Serve dynamic content1
      if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // !S_ISREG = 일반 파일이 아니고, !S_IXUSR = 실행 권한이 없을 때
          clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");

          return;
      }
      serve_dynamic(fd, method, filename, cgiargs);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // Build the HTTP response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // Print the HTTP response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) // tiny server에서 헤더는 읽고 버린다
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) { // \r - carriage return , \n - line feed
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  // 현재 디렉토리가 정적 컨텐츠를 위한 home directory고 ./cgi-bin이 동적 컨텐츠를 위한 home directory
  if (!strstr(uri, "cgi-bin")) { //Static content //"cgi-bin"이라는 문자열을 포함하는 모든 uri는 동적 컨텐츠일 것으로 예측
      strcpy(cgiargs, ""); //정적 컨텐츠니까 CGI argument string을 비우고. strcpy = cgiargs를 ""로 대체
      strcpy(filename, "."); // filename을 .으로 변환
      strcat(filename, uri); // .으로 변환된 filename 뒤에 uri 붙이기
      if (uri[strlen(uri)-1] == '/') // 만약 uri가 /로 끝나면 뒤에 default filename 붙이기
        strcat(filename, "home.html"); //default filename = ./home.html
      return 1;
  }
  else { //Dynamic content
      ptr = index(uri, '?'); //'?'가 위치한 index에 ptr두고,
      if (ptr){
        strcpy(cgiargs, ptr+1); // 인자들 추출
        *ptr ='\0'; // 포인터를 종단문자로 변경
      }
      else
        strcpy(cgiargs, "");
      strcpy(filename, ".");
      strcat(filename, uri);
      return 0;
  }
}

void serve_static(int fd, char *method, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  rio_t rio;

  // Send response headers to client
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response header: \n");
  printf("%s", buf);

  if (!(strcasecmp(method, "HEAD"))) {  //GET만 받는다. 아닐시 연결을 끊고 다음 연결 요청을 기다림 (HEAD 만들기 숙제)
    return;
  }

  // Send response body to client
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = malloc(filesize);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Rio_readinitb(&rio, srcfd);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
}

//get filetype
void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpg"))
        strcpy(filetype, "video/mpg");
    else
        strcpy(filetype, "text/plain");    
}

void serve_dynamic(int fd, char *method, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};
  // *emptylist[] = { method, NULL } 우리팀 방법

  //Return first part of HTTP response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0) { //Child
      //real server would set all CGI vars here
      setenv("QUERY_STRING", cgiargs, 1);
      setenv("REQUEST_METHOD", method, 1); // HEAD 요청을 위해서 Method도 전달
      Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
      Execve(filename, emptylist, environ); // CGI program 실행
  }
  Wait(NULL); //부모가 자식을 기다렸다가 거둬들인다
}



