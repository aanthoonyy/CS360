#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "csapp.h"

#define LISTEN_PORT 23361
#define BUFFER_SIZE 4096
#define FORWARD_PORT 80
#define FORWARD_HOST "17.253.31.201" // IP address of http://captive.apple.com/

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void parse_uri(char* uri, char* host, char* port, char* path){
    char *begin;
    begin = strstr(uri, "//");
    if(begin)
    {
    begin += 2;
    char *hostn = strchr(begin, '/');
    char *portn = strchr(begin, ':');

    if(hostn && portn && ( portn < hostn )){
        // strncpy(hostname, begin, host - begin);
        // begin = host + 1;
        // host = strstr(begin, "/");
        // strncpy(port, begin, host - begin);
        // begin = host;
        // strcpy(port, begin);
        int offset = portn - begin;
        strncpy(host, begin, offset);
        host[offset] = '\0';

        int port_offset = hostn - portn - 1;
        strncpy(port, portn + 1, port_offset);
        port[port_offset] = '\0';

        strcpy(path, hostn);
    }
    else if (hostn){
        int offset = hostn - begin;
        strncpy(host, begin, offset);
        host[offset] = '\0';
        strcpy(port, "80");
        strcpy(path, hostn);
    }
    else {
        strcpy(host, begin);
        strcpy(port, "80");
        strcpy(path, "/");
    }
    }
}

int main(int argc, char **argv) {
    printf("0");
    int listen_sock, client_sock, web_sock;
    struct sockaddr_storage client_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    unsigned int clientLen;
    rio_t rio;

    // Initialize listen socket
    listen_sock = Open_listenfd(argv[1]);

    char method[100] = "", uri[100] = "", version[100] = "";
    printf("1");
while (1) { 

    clientLen = sizeof(client_addr);
    // Accept connection
    client_sock = Accept(listen_sock, (struct sockaddr*)&client_addr, &clientLen);
    if (client_sock < 0) {
        perror("Accept failed");
        return 1;
    }
    printf("2");

    // Initialize forward socket
    // forward_sock = socket(AF_INET, SOCK_STREAM, 0);
    // if (forward_sock < 0) {
    //     perror("Socket creation failed");
    //     close(client_sock); // Ensure the client socket is closed before exiting
    //     return 1;
    // }

    // Connect to forward addresss
    Rio_readinitb(&rio, client_sock);
    if (Rio_readlineb(&rio, buffer, BUFFER_SIZE) < 0) {
        perror("Failed to read from client");
        close(client_sock);
        continue;
    }
    printf("Buffer: %s", buffer);
    printf("3");
    sscanf(buffer, "%s %s %s", method, uri, version);
    printf("URI: %s", uri);
    // METHOD COULD GET OR STRING OR POST WE ARE LOOKING FOR GET
    if (strcasecmp(method, "GET")) {
        printf("Proxy does not implement this method\n");
        close(client_sock);
        //return 1;
    }
    else { // fowrward the request to the server
        printf("4");
        char hostname[100] = "", path[100] = "", port[100] = "";
                printf("hostname: %s\n", hostname);
        printf("path: %s\n", path);
        printf("port: %s\n", port);
        parse_uri(uri, hostname, port, path);
        printf("5");
        printf("hostname: %s\n", hostname);
        printf("path: %s\n", path);
        printf("port: %s\n", port);
        if (hostname[0] != '0' && path[0] != '0'){
            rio_t web_rio;
            web_sock = Open_clientfd(hostname, port);
            Rio_readinitb(&web_rio, web_sock);
            printf("6");
            if (web_sock < 0) {
                perror("Failed to connect to forward address");
                close(client_sock);
                continue;
            }
            printf("7");
            char request[BUFFER_SIZE] = "";
            sprintf(request, "GET %s HTTP/1.0\r\n\r\n", path);
            printf("Forwarding request to %s\n", request);
            printf("8");
            Rio_writen(web_sock, request, strlen(request));
            printf("8.5");
            printf("Forwarded request to %s\n", buffer);

            char response[BUFFER_SIZE] = "";
            printf("9");
            while ((bytes = Rio_readlineb(&web_rio, response, BUFFER_SIZE)) > 0) {
                Rio_writen(client_sock, response, bytes);
            }
            printf("10");
            close(web_sock);
        }
    close(client_sock);

    }

}
    // Close sockets
    close(listen_sock);

    return 0;
}

