#ifndef util_h
#define util_h

#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h> 
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <iostream>
#include <climits>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <shared_mutex>
#include <thread>

#include "exception.h"

#define MYPORT 12345
#define BACKLOG 10
#define BUFFERSIZE 1024
#define CACHESIZE 2 // 32
#define DEFAULTAGE (60)// 1 min for test  (60 * 60 * 24 * 30) // a month, in case of heuristic lifetime

#define _LOG(x) (log((x)))
#define LOG(x) (log((x), id))

using namespace std;

extern const string response_200, response_400, response_502;

inline void log(string s){
    fprintf(stdout, "no-id: %s\n", s.c_str());
    fflush(stdout);
}

inline void log(string s, int id){
    fprintf(stdout, "%d: %s\n", id, s.c_str());
    fflush(stdout);
}

inline string detachPortIfAny(string &url){
    string port;

    if(url.find_last_of(':') != string::npos){
        port = url.substr(1+url.find_last_of(':'));
        for(auto i:port){
            if(!isdigit(i)){
                port.clear();
                break;
            }
        }
        if(!port.empty()){
            url = url.substr(0, url.find_last_of(':'));
        }
    }

    return port;
}

inline struct tm dateToTm(string &date){
    struct tm ret;
    if(strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &ret) == NULL){
        time_t now;
        time(&now);
        struct tm *now_tm = gmtime(&now);
        mktime(now_tm);
        _LOG("WARNING a invalid date is parsed");
        return *now_tm;
    }
    return ret;
}

inline string tmToDate(struct tm &t){
    char buffer[64] = {0};
    strftime(buffer, 64, "%a, %d %b %Y %H:%M:%S GMT", &t);
    return string(buffer);
}

inline string curentDate(){
    time_t now;
    time(&now);
    struct tm *now_tm = gmtime(&now);
    mktime(now_tm);
    return tmToDate(*now_tm);
}

inline void sendMessage(const string &message, int fd){
    int sent = 0, target = message.size();
    while(sent < target){
        int cur = send(fd, message.c_str()+sent, target-sent, 0);
        if(cur < 0){
            throw SendException(strerror(errno));
        }
        sent += cur;
    }
}

inline string oneLineResponse(const string &message){
    return message.substr(0, message.size() - 4);
}

#endif /* util_h */