#ifndef proxy_h
#define proxy_h

#include "parser.h"
#include "cache.h"

extern Cache cache;

class Proxy{
private:
    string server_address, server_port;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    const int cfd; // cliend fd
    int sfd; // server fd
    char buffer[BUFFERSIZE];

    void _handleGet(const RequestParser &, string);
    void revalidate(const RequestParser &, const string &);
    string receiveMessage(int);

public:
    Proxy(string address, string port, int cfd): 
    server_address(address), server_port(port), cfd(cfd){
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
    }

    ~Proxy(){
        freeaddrinfo(servinfo);
        close(sfd);
        close(cfd);
    }

    void init();
    void handleConnect(int);
    void handlePost(const RequestParser &);
    void handleGet(const RequestParser &);
};

#endif /* proxy_h */