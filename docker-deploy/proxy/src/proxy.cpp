#include "util.h"
#include "proxy.h"
#include "parser.h"

void Proxy::init(){
    int status;
    detachPortIfAny(server_address);
    if(status = getaddrinfo(server_address.c_str(), server_port.c_str(), &hints, &servinfo)){
        throw GetAddressInfoException("address: " + server_address + ", port: " + server_port + "\nReason: " + strerror(errno));
    }

    sfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sfd < 0){
        throw SocketException("address: " + server_address + ", port: " + server_port + "\n" + strerror(errno));
    }

    if(connect(sfd, servinfo->ai_addr, servinfo->ai_addrlen)){
        throw ConnectException("address: " + server_address + ", port: " + server_port + "\n" + strerror(errno));
    }
}

void Proxy::handleConnect(int id){
    sendMessage(response_200, cfd);
    LOG("Responding " + oneLineResponse(response_200));
    fd_set allset, rset;
    int maxfd = max(cfd, sfd);
    FD_ZERO(&allset);
    FD_SET(cfd, &allset);
    FD_SET(sfd, &allset);
    while(1){ 
        rset = allset;
        if(select(maxfd+1, &rset, NULL, NULL, NULL) < 0){
            throw SelectException(strerror(errno));
        }
        if(FD_ISSET(cfd, &rset)){ 
            // the outer loop ensures the messages are receved completely
            string msg = receiveMessage(cfd);  
            if(msg.empty()) break;
            else sendMessage(msg, sfd);                
        } else if(FD_ISSET(sfd, &rset)){
            string msg = receiveMessage(sfd);
            if(msg.empty()) break;
            else sendMessage(msg, cfd);
        }
    }
    LOG("Tunnel closed");
}

void Proxy::handlePost(const RequestParser &req){
    int id = req.getId();
    string message = req.reconstruct();
    LOG("Requesting " + req.getStartLine() + " from " + req.getHost());
    sendMessage(message.c_str(), sfd);
    ResponseParser res(sfd, id);
    try{
        res.fetchHead();
        res.parseHead();
        res.fetchContent();
    } catch(FetchParseException &e){
        sendMessage(response_502, cfd);
        LOG("Responding " + oneLineResponse(response_502));
    }
    LOG("Received " + res.getStartLine() + " from " + req.getHost());
    LOG("Responding " + res.getStartLine());
    sendMessage(res.reconstruct(), cfd);
    // int received;
    // while(received = recv(sfd, buffer, BUFFERSIZE, 0)){
    //     if(received < 0){
    //         fprintf(stderr, "error receive from server: %s\n", strerror(errno));
    //         return;
    //     }
    //     sendMessage(string(buffer, received), cfd);
    // }
}

void Proxy::_handleGet(const RequestParser &req, string message = ""){
    int id = req.getId();
    if(message.empty())
        message = req.reconstruct();
    string url = req.constructEffectiveUrl();
    LOG("Requesting " + req.getStartLine() + " from " + req.getHost());
    sendMessage(message.c_str(), sfd);
    ResponseParser res(sfd, id);
    try{
        res.fetchHead();
        res.parseHead();
        res.fetchContent();
    } catch(FetchParseException &e){
        sendMessage(response_502, cfd);
        LOG("Responding " + oneLineResponse(response_502));
    }
    LOG("Received " + res.getStartLine() + " from " + req.getHost());
    res.parseFields();
    auto res_result = cache.checkResponse(res);
    switch(res_result.status){
        case 0:{
            LOG("not cacheable because " + res_result.detail);
            break;
        }
        case 1:{
            cache.save(url, res);
            LOG("cached, expires at " + res_result.detail + " current time: " + curentDate());
            break;
        }
        case 2:{
            cache.save(url, res);
            LOG("cached, but requires validation");
            break;
        }
        default:break;
    }
    sendMessage(res.reconstruct(), cfd);
    LOG("Responding " + res.getStartLine());
}

void Proxy::handleGet(const RequestParser &req){
    int id = req.getId();
    auto check_result = cache.checkRequest(req);
    switch(check_result.status){
        case 0:{
            LOG("not in cache");
            _handleGet(req);
            break;
        }
        case 1:{
            LOG("in cache, but expired at " + check_result.detail);
            _handleGet(req);
            break;
        }
        case 2:{
            LOG("in cache, requires validation");
            revalidate(req, check_result.detail);
            break;
        }
        case 3:{
            LOG("in cache, valid");
            auto target = cache.get(req, id);
            sendMessage(target.first, cfd);
            LOG("Responding " + target.second);
            break;
        }
        case 4:{
            LOG("NOTE in cache, but the request states it does not accept cache");
            _handleGet(req);
            break;
        }
        default:break;
    }
}

void Proxy::revalidate(const RequestParser &req, const string &etag){
    string message = req.getStartLine() + "\r\nHost: " + req.getHost() + "\r\nIf-None-Match: " + etag + "\r\n\r\n";
    _handleGet(req, message);
}

string Proxy::receiveMessage(int fd){
    string message;
    int received = recv(fd, buffer, BUFFERSIZE, 0);
    if(received < 0){
        throw RecvException("connect receive", strerror(errno));
    } 
    message.append(buffer, received);
    return message;
}