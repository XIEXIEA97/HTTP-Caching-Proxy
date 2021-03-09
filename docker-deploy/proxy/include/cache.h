#ifndef cache_h
#define cache_h

#include "parser.h"

class CheckResult{
public:
    int status;
    string detail;
    CheckResult(int _status, string _detail): status(_status), detail(_detail){}
    CheckResult(int _status): status(_status){}
};

class Cache{
private:
    int capacity;
    unordered_map<string, ResponseParser> m;

    pair<string, string> _get(const RequestParser&);

public:
    Cache(int c):capacity(c){}
    CheckResult checkRequest(const RequestParser&);
    CheckResult checkResponse(ResponseParser&);
    pair<string, string> get(const RequestParser&, int);
    void save(string, const ResponseParser&);
};


#endif /* cache_h */