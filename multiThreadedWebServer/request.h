#ifndef __REQUEST_H__

typedef struct {
    int connfd;
    
    int is_static;
    int stat_return;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
} request_t;

void requestHandle(int fd, request_t request);
int requestParseURI(char *uri, char *filename, char *cgiargs);
void requestReadhdrs(rio_t *rp);

#endif
