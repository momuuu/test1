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
void send_http_header(int, int, int);
void clear_header(int);
void send_static_file(int, char*, int);
void execute_cgi(int, const char *, const char*, const char*);

void execute_cgi(int client, const char *path, const char *method, const char *query_string){
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    if (strcasecmp(method, "GET") == 0){
        clear_header(client);
    }
    else{
        numchars = getLine(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf)){
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = getLine(client, buf, sizeof(buf));
        }
        if (content_length == -1) {
            send_static_file(client, "500.html", 500);
            return;
        }
    }


    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    if (pipe(cgi_output) < 0) {
        send_static_file(client, "500.html", 500);
        return;
    }
    if (pipe(cgi_input) < 0) {
        send_static_file(client, "500.html", 500);
        return;
    }

    if ( (pid = fork()) < 0 ) {
        send_static_file(client, "500.html", 500);
         return;
    }

    if (pid == 0){
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }else {

            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }

        execl(path, path, NULL);
        exit(0);
    }else {   
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
        for (i = 0; i < content_length; i++) {
            recv(client, &c, 1, 0);
            write(cgi_input[1], &c, 1);
        }
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);

        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

void send_static_file(int client, char *path,  int code){
    FILE *file = NULL;
    struct stat st;
    if(stat(path, &st) == -1){
        code = 404;
        sprintf(path, "WWW/404.html");
        stat(path, &st);
    }
    file = fopen(path, "r");
    send_http_header(client, code, st.st_size);
    char buf[1024];
    fgets(buf, sizeof(buf), file);
    while(!feof(file)){
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), file);
    }
    
}

void clear_header(int client){
    char buf[128];
    memset(buf, 0, sizeof(buf));
    int n = 1;
    while((n > 0) && strcmp("\n", buf)){
        n = getLine(client, buf, sizeof(buf));
    }
}

void send_http_header(int client, int code, int cont_len){
    char buf[1024];
    char msg[64];
    if(code == 200){
        sprintf(msg, "OK");
    }else if(code == 404){
        sprintf(msg, "NOT FOUND");
    } else if(501 == code){
        sprintf(msg, "Method Not Implemented");
    }else if(500 == code){
        sprintf(msg, "Internal Server Error");
    }else if(400 == code){
        sprintf(msg, "BAD REQUEST");
    }
    sprintf(buf, "HTTP/1.0 %d %s\r\n", code, msg);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html;charset=UTF-8\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Length: %d\r\n\r\n", cont_len);
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
    int code = 200;
    char buf[1024];
    char method[256];
    char url[512];
    char path[512];
    size_t i = 0;
    size_t j = 0;
    char *query_string = NULL;
    int cgi = 0;
    getLine(client, buf, 1024);
    while (!isspace((int)buf[j]) && (i < sizeof(method) - 1)){
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")){
        code = 501;
    }
    if (strcasecmp(method, "POST") == 0)
        cgi = 1;
    
    i = 0;
    while(isspace((int)buf[j]) && (j < sizeof(buf)))
        j++;
    while (!isspace((int)buf[j]) && (i < sizeof(url) - 1)){
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';
    if(strcasecmp(method, "GET") == 0){
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if(*query_string == '?'){
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }
    if (code == 501){
        sprintf(url, "/501.html");
    }
    sprintf(path, "WWW%s", url);
    
    if(path[strlen(path)-1] == '/')
        strcat(path, "index.html");
    if (!cgi){
        clear_header(client);
        send_static_file(client, path,code);
    }else{
        execute_cgi(client, path, method, query_string);
    }
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
