/* $begin proxymain */
/*
 * proxy.c - A simple http Web prxoy n between a Web browser and an end server.
 * Instead of contacting the end server directly to get a Web page, the browser
 * contacts the proxy, which forwards the request on to the end server.
 * When the end server replies to the proxy, the proxy sends the reply on to the
 * browser.
 *
 * created 04/2025 zhanglei
 */

#include <stdio.h>

#include "csapp.h"
#include "sbuf.h"

#define NTHREADS  4
#define SBUFSIZE  16

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, char *host, int *port, char *filename);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptors */



int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    sbuf_init(&sbuf, SBUFSIZE); //line:conc:pre:initsbuf
    for (int i = 0; i < NTHREADS; i++)  /* Create worker threads */ //line:conc:pre:begincreate
	    Pthread_create(&tid, NULL, thread, NULL); 

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:proxy:accept
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }

    printf("%s", user_agent_hdr);
    return 0;
}

/* $end proxymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char request_str[MAXLINE];
    char filename[MAXLINE];
    char host[20], port_str[20];
    int port;
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  // line:netp:doit:readrequest
        return;
    strcpy(request_str, buf);
    printf("request_str = %s", request_str);
    sscanf(buf, "%s %s %s", method, uri, version);  // line:netp:doit:parserequest
    printf("request: %s %s %s\n", method, uri, version);
    if (strcasecmp(method, "GET")) {  // line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented", "Proxy does not implement this method");
        return;
    }
    parse_uri(uri, host, &port, filename);
    printf("host: %s, port: %d, filename: %s\n", host, port, filename);

    static const char *connection_key = "Connection";
    static const char *user_agent_key= "User-Agent";
    static const char *proxy_connection_key = "Proxy-Connection";
    char request_header[MAXLINE];
    sprintf(request_header, "GET %s HTTP/1.0\r\n", filename);
    char other_header[MAXLINE];
    while (Rio_readlineb(&rio, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
        if (!strncasecmp(buf, "HOST", strlen("HOST"))) {
            sprintf(request_header, "%s%s", request_header, buf);
            continue;
        }
        if (strncasecmp(buf, connection_key, strlen(connection_key))
                && strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
                && strncasecmp(buf, user_agent_key, strlen(user_agent_key))) {
            strcat(other_header, buf);
        }
    }
    // GET /Makefile HTTP/1.1 \r\nHost: localhost\r\nConnection: close\r\n\r\n
    // curl -v --proxy http://localhost:9008 http://localhost:9009/
    sprintf(request_header, "%s%s%s%s%s%s", 
            request_header,
            user_agent_hdr,
            "Connection: close\r\n",
            "Proxy-Connection: close\r\n",
            other_header,
            "\r\n");
    printf("request_header:\n%s\n", request_header);

    sprintf(port_str, "%d", port);

    int server_fd;
    rio_t server_rio;
    server_fd = Open_clientfd(host, port_str);
    Rio_readinitb(&server_rio, server_fd);

    Rio_writen(server_fd, request_header, strlen(request_header));

    /* Receive mgs from end server and send to client */
    size_t n;
    while ((n = rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
        printf("proxy received %ld bytes, then send to client %d\n", n, fd);
        Rio_writen(fd, buf, n);
    }

    Close(server_fd);

}
/* $end doit */

void parse_uri(char *uri, char *host, int *port, char *filename) {
    char *ptr = strstr(uri, "//");
    if (ptr) {
        uri = ptr + 2;
    }
    printf("uri = %s\n", uri);
    ptr = strstr(uri, ":");
    if (ptr) {
        *ptr = 0;
        strcpy(host, uri);
        sscanf(ptr + 1, "%d%s", port, filename);
    } else {
        ptr = strstr(uri, "/");
        if (ptr) {
            *ptr = 0;
            strcpy(host, uri);
            *ptr = '/';
            strcpy(filename, ptr);
        } else {
            strcpy(host, uri);
        }
    }
}


/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf,
            "<body bgcolor="
            "ffffff"
            ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */

void *thread(void *vargp) 
{  
    Pthread_detach(pthread_self()); 
    while (1) { 
	int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */ //line:conc:pre:removeconnfd
	doit(connfd);                /* Service client */
	Close(connfd);
    }
}
