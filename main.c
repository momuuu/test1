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

void *accept_request(void*);
int init_listen_sock(u_short *port);

void error_die(const char* msg){
    perror(msg);
    exit(1);
}

void *accept_request(void* client_sock){
    int client = *(int *)client_sock;
    char buf[1024];
    char buffer[1024];
    recv(client, buf, 1024, 0);
    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    send(client, buffer, strlen(buffer), 0);
    sprintf(buffer, "Content-Type: text/html\r\n");
    send(client, buffer, strlen(buffer), 0);
    sprintf(buffer, "\r\n");
    send(client, buffer, strlen(buffer), 0);
    sprintf(buffer, "testing");
    send(client, buffer, strlen(buffer), 0);
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
