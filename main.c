#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#define SERVER "Server: simple web server by momu\r\n"


void *accept_request(void*);
int init_listen_sock(u_short *port);
int getLine(int, char* , int);
void send_httpok_header(int);
void clear_header(int);

void clear_header(int client){
    char buf[128];
    memset(buf, 0, sizeof(buf));
    int n = 1;
    while((n > 0) && strcmp("\n", buf)){
        n = getLine(client, buf, sizeof(buf));
    }
}

void send_httpok_header(int client){
    char buf[1024];
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Content-Type: text/html;charset=UTF-8\r\n\r\n");
    send(client, buf, strlen(buf), 0);
}

int getLine(int client, char *buf, int buf_size){
    int i = 0;
    char c = '\0';
    int n = 0;
    while((i < buf_size - 1) && (c != '\n')){
        n = recv(client, &c, 1, 0);
        if (n > 0){
            if(c == '\r'){
                n = recv(client, &c, 1, MSG_PEEK);
                if((n > 0) && (c == '\n'))
                    recv(client, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i ++;
        }else {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
}

void error_die(const char* msg){
    perror(msg);
    exit(1);
}

void *accept_request(void* client_sock){
    int client = *(int *)client_sock;
    char buf[1024];
    char method[256];
    size_t i = 0;
    size_t j = 0;
    getLine(client, buf, 1024);
    while (!isspace((int)buf[j]) && (i < sizeof(method) - 1)){
        method[i] = buf[j];
        i++;
        j++;
    }
    clear_header(client);
    method[i] = '\0';
    send_httpok_header(client);
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")){
        sprintf(method, "unable method");
    }
    send(client, method, strlen(method), 0);
   
    close(client);
    return NULL;
}

int init_listen_sock(u_short *port){
    int listen_sock = 0;
    struct sockaddr_in listen_addr;
    
    listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1)
        error_die("error on socket()");

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(*port);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(listen_sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr) ) < 0)
        error_die("error on bind()");
    
    if (listen(listen_sock, 16) < 0)
        error_die("error on listen()");

    return listen_sock;
}

int main(int argc, char **argv){
    if(argc != 2)error_die("input a port");
    int listen_sock = -1;
    u_short port = (u_short)atoi(argv[1]);
    int client_sock = -1;
    struct sockaddr_in client_addr;
    socklen_t  client_addr_len = sizeof(client_addr);
    pthread_t pt;
    
    listen_sock = init_listen_sock(&port);
    printf("running on port %d\n", port);
    
    while(1){
        client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock == -1)
            error_die("error on accept");
        if (pthread_create(&pt, NULL, accept_request, (void*)&client_sock) != 0)
            perror("error on pthread_create");
    }
    close(listen_sock);
    
    return 0;
}
