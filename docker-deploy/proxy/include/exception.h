#ifndef exception_h
#define exception_h

#include <string>
#include <string.h>
#include <exception>

using namespace std;

class RecvException: public exception{
private:
    string message;
public:
    explicit RecvException(const string &_where, const string &_message): message(_where + " receive failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class SendException: public exception{
private:
    string message;
public:
    explicit SendException(const string &_message): message("send failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class SocketException: public exception{
private:
    string message;
public:
    explicit SocketException(const string &_message): message("socket failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class ConnectException: public exception{
private:
    string message;
public:
    explicit ConnectException(const string &_message): message("connect failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class GetAddressInfoException: public exception{
private:
    string message;
public:
    explicit GetAddressInfoException(const string &_message): message("getaddrinfo failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class SelectException: public exception{
private:
    string message;
public:
    explicit SelectException(const string &_message): message("select failed: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class FetchParseException: public exception{};

class HeadFetchException: public FetchParseException{
    const char* what() const noexcept override{
        return "failed to fetch http head";
    }
};

class ChunkFetchException: public FetchParseException{
    const char* what() const noexcept override{
        return "does not receive data as chunk size claimed.";
    }
};

class ContentFetchException: public FetchParseException{
    const char* what() const noexcept override{
        return "does not receive data as content length claimed.";
    }
};

class BadRequestException: public FetchParseException{
private:
    string message;
public:
    explicit BadRequestException(const string &_message): message("bad request: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

class BadResponseException: public FetchParseException{
private:
    string message;
public:
    explicit BadResponseException(const string &_message): message("bad response: " + _message){}
    const char* what() const noexcept override{
        return message.c_str();
    }
};

#endif /* exception_h */