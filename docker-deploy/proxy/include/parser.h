#ifndef parser_h
#define parser_h

class Parser{
protected:
    int fd, id;
    char buffer[BUFFERSIZE];
    string head_message, body_message, start_line;
    string version, port, host;
    unordered_map<string, string> fields;
    int content_length = 0;
    bool is_chunked = false;

    void fetchChunkedContent();
    void fetchRemainingContent();

public:
    Parser(){}
    Parser(int _fd, int _id): fd(_fd), id(_id){}

    Parser& operator=(const Parser &p){
        fd = p.id;
        id = p.id;
        head_message = p.head_message;
        body_message = p.body_message;
        start_line = p.start_line;
        version = p.version;
        port = p.port;
        host = p.host;
        fields = p.fields;
        content_length = p.content_length;
        is_chunked = p.is_chunked;
        return *this;
    }

    ~Parser(){}

    void fetchHead();
    inline void fetchContent(){is_chunked ? fetchChunkedContent() : fetchRemainingContent();}
    void parseHead();
    virtual void parseStartLine(stringstream &) = 0;
    virtual void showHead() = 0;
    string reconstruct()const;
    // void compareHead();
    int getFd() const{return fd;}
    int getId() const{return id;}
    string getStartLine() const{return start_line;}
    string getHeadMessage() const{return head_message;}
    string getBodyMessage() const{return body_message;}
    string getVersion() const{return version;}
    string getPort() const{return port;}
    string getHost() const{return host;}
    unordered_map<string, string> getFields() const{return fields;}
    int getContentLength() const{return content_length;}
    bool isChunked() const{return is_chunked;}
};

class RequestParser: public Parser{
private:
    string method, request_target;

public:
    RequestParser(int fd, int id):Parser(fd, id){};
    void parseStartLine(stringstream &);
    void showHead();
    string constructEffectiveUrl() const;
    string getMethod() const{return method;}
    string getRequestTarget() const{return request_target;}
};

class ResponseParser: public Parser{
private:
    string status_code, reason_phrase, etag;
    struct tm lifetime;

public:
    ResponseParser(){}
    ResponseParser(int fd, int id):Parser(fd, id){}

    ResponseParser& operator=(const ResponseParser &p){
        Parser::operator=(p);
        status_code = p.status_code;
        reason_phrase = p.reason_phrase;
        etag = p.etag;
        lifetime = p.lifetime;
        return *this;
    }

    void setId(int _id){id = _id;};
    void parseStartLine(stringstream &);
    void parseFields();
    void showHead();
    string getStatusCode() const{return status_code;}
    string getEtag() const{return etag;}
    string getReasonPhrase() const{return reason_phrase;}
    struct tm getLifeTime() const{return lifetime;}
};

#endif /* parser_h */