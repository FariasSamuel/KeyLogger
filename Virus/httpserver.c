#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
struct httpserver
{
    int domain;
    int port;
    int service;
    int protocol;
    int backlog;
    unsigned long interface;

    int socket;
    struct sockaddr_in adress;
    void (*launch)(struct httpserver *server);
};

void launch(struct httpserver *server){
    char buffer[1600];
    while(1){
        int addrlen = sizeof(server->adress);
        int new_socket = accept(server->socket,(struct sockaddr*)&server->adress,(socklen_t*)&addrlen);
        ssize_t bytesRead = read(new_socket,buffer,1600-1);
        if(bytesRead >= 0){
            buffer[bytesRead] = '\0';
            puts(buffer);
        }else{
            perror("error na leitura do buffer\n");
        }
        if (strncmp(buffer, "POST", 4) == 0) {
            // Encontrar o corpo (após \r\n\r\n)
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4; // pular \r\n\r\n
                printf("\n=== POST BODY ===\n%s\n", body);
            }
        }
        char *response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                         "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "<head>\r\n"
                         "<title>Testing Basic HTTP-SERVER</title>\r\n"
                         "</head>\r\n"
                         "<body>\r\n"
                         "Testando!\r\n"
                         "</body>\r\n"
                         "</html>\r\n";
        write(new_socket, response, strlen(response));
        close(new_socket);
    }
}

struct httpserver server_creator(int domain, int port, int service, int protocol, int backlog, unsigned long interface,void (*launch)(struct httpserver *server)){
    struct httpserver server;

    server.domain = domain;
    server.port = port;
    server.service = service;
    server.protocol = protocol;
    server.backlog = backlog;

    server.adress.sin_family = domain;
    server.adress.sin_port = htons(port);
    server.adress.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain,service,protocol);
    if(server.socket < 0){
        perror("Failed to initialize/connect to socket...\n");
        exit(-1);
    }

    if (bind(server.socket, (struct sockaddr*)&server.adress, sizeof(server.adress)) < 0) {
        perror("Failed to bind socket...\n");
        exit(-1);
    }

    if (listen(server.socket, server.backlog) < 0) {
        perror("Failed to start listening...\n");
        exit(-1);
    }

    server.launch = launch;
    return server;
}

int main(){
    struct httpserver server = server_creator(AF_INET, 81, SOCK_STREAM, 0, 10, INADDR_ANY, launch);
    server.launch(&server);
    return 0;
    return 0;
}