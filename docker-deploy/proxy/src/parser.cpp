#include "util.h"
#include "parser.h"

void Parser::fetchHead(){
    size_t headend;
    int deli = 4;
    while((headend = head_message.find("\r\n\r\n")) == string::npos){
        int received = recv(fd, buffer, BUFFERSIZE, 0);
        if(received < 0){
            throw RecvException("head", strerror(errno));
        } else if(received > 0){
            head_message.append(buffer, received);
        } else break;
    }

    // \n\n might works, like if send a request to google using \n\n as the end of head when the reqeust comes from macOS ... some websites does not accepst that 
    if(headend == string::npos){
        if((headend = head_message.find("\n\n")) == string::npos){
            head_message.clear();
            throw HeadFetchException();
        }
        deli = 2;
    }

    body_message = head_message.substr(min(int(headend)+deli, int(head_message.size())));
    head_message.erase((int)head_message.size() - (int)body_message.size() - deli);
}

void Parser::fetchChunkedContent(){
    string tail;
    while(tail.find("0\r\n\r\n") == string::npos){
        int received = recv(fd, buffer, BUFFERSIZE, 0);
        if(received < 0){
            throw RecvException("fetch chunk", strerror(errno));
        } else if(received > 0){
            tail.append(buffer, received);
        } else {
            throw ChunkFetchException();
        }
    }
    body_message += tail;
}

void Parser::fetchRemainingContent(){
    int left = content_length - (int)body_message.size();
    while(left > 0){
        int received = recv(fd, buffer, BUFFERSIZE, 0);
        if(received < 0){
            throw RecvException("body", strerror(errno));
        } else if(received > 0){
            left -= received;
            body_message.append(buffer, received);
        } else {
            throw ContentFetchException();
        }
    }
}

string Parser::reconstruct() const{
    return head_message + "\r\n\r\n" + body_message;
    // string ret = start_line + "\r\n";
    // for(auto [k, v]:fields){
    //     ret += k + ": " + v + "\r\n";
    // }
    // return ret + "\r\n" + body_message;
}

void Parser::parseHead(){
    stringstream ss(head_message);
    string line;
    int lineid = 0;
    while(getline(ss, line)){
        if(line.back() == '\r') line.pop_back();
        if(line.empty()) break;
        if(!lineid) start_line = line;
        stringstream sline(line);
        if((lineid++) == 0){    // head line
            parseStartLine(sline);
        } else{
            string name, value;
            if(getline(sline, name, ':') && getline(sline, value)){
                if(value.back() == '\r') value.pop_back();
                fields[name] = value.substr(1); // the space after colon
            }
        }
        if(fields.count("Host")) host = fields["Host"];
        if(fields.count("Transfer-Encoding") && fields["Transfer-Encoding"].find("chunked") != string::npos) is_chunked = true;
        if(fields.count("Content-Length")) content_length = stoi(fields["Content-Length"]);
        if(port.empty()) port = "80";
    }
}

void RequestParser::showHead(){
    fprintf(stdout, "\nHeader info:\n");
    fprintf(stdout, "method: %s\n", method.c_str());
    fprintf(stdout, "target: %s\n", request_target.c_str());
    fprintf(stdout, "http version: %s\n", version.c_str());
    fprintf(stdout, "port: %s\n", port.c_str());
    for(auto [k, v]:fields){
        fprintf(stdout, "field name: %s, with value: %s\n", k.c_str(), v.c_str());
    }
    fprintf(stdout, "\nEnd of header info.\n");
}

void RequestParser::parseStartLine(stringstream &sline){
    getline(sline, method, ' ');
    if(method != "GET" && method != "HEAD" && method != "POST" && method != "PUT" 
    && method != "DELETE" && method != "CONNECT" && method != "OPTIONS" && method != "TRACE"){
        throw BadRequestException("unexpected method");
    }

    getline(sline, request_target, ' ');
    if(request_target.empty()){
        throw BadRequestException("no request target");
    }

    port = detachPortIfAny(request_target);

    getline(sline, version, ' ');
    if(version.empty()){
        throw BadRequestException("no http version");
    }
}

string RequestParser::constructEffectiveUrl() const{
    string scheme, authority, CPQC = ""; //combined path and query component
    if(request_target.rfind("https", 0) == 0){
        scheme = "https";
    } else if(request_target.rfind("http", 0) == 0){
        scheme = "http";
    } else if(port == "443"){
        scheme = "https";
    } else scheme = "http";
    if(request_target.find("//") != string::npos){
        authority = request_target.substr(request_target.find("//")+2);
        if(authority.find("/") != string::npos){
            CPQC = authority.substr(request_target.find("/"));
            authority = authority.substr(0, request_target.find("/"));
        } else if(authority.find("#") != string::npos){
            CPQC = authority.substr(request_target.find("#"));
            authority = authority.substr(0, request_target.find("#"));
        } else if(authority.find("?") != string::npos){
            CPQC = authority.substr(request_target.find("?"));
            authority = authority.substr(0, request_target.find("?"));
        }
    } else if(!host.empty()){
        authority = host;
    }
    if(request_target.rfind("/", 0) == 0){
        CPQC = request_target;
    }

    return scheme + "://" + authority + CPQC;
}

void ResponseParser::parseStartLine(stringstream &sline){
    getline(sline, version, ' ');
    if(version.empty()){
        throw BadResponseException("no http version in response");
    }

    getline(sline, status_code, ' ');
    if(status_code.empty()){
        throw BadResponseException("no status code in response");
    }

    getline(sline, reason_phrase, ' ');
    if(reason_phrase.empty()){
        throw BadResponseException("no reason phrase in response");
    }
}

void ResponseParser::showHead(){
    fprintf(stdout, "\nHeader info:\n");
    fprintf(stdout, "version: %s\n", version.c_str());
    fprintf(stdout, "status code: %s\n", status_code.c_str());
    fprintf(stdout, "reason phrase: %s\n", reason_phrase.c_str());
    fprintf(stdout, "port: %s\n", port.c_str());
    for(auto [k, v]:fields){
        fprintf(stdout, "field: %s, with value: %s\n", k.c_str(), v.c_str());
    }
    fprintf(stdout, "\nEnd of header info.\n");
}

void ResponseParser::parseFields(){
    int age = DEFAULTAGE;
    if(fields.count("Cache-Control") && fields.count("Date")){
        string cc = fields["Cache-Control"];
        size_t pos;
        if(pos = cc.find("s-maxage") != string::npos){
            size_t begin, end;
            begin = cc.find("=", pos) + 1;
            end = cc.find_last_not_of("0123456789", begin);
            age = stoi(cc.substr(begin, end - begin));
        } else if(pos = cc.find("max-age") != string::npos){
            size_t begin, end;
            begin = cc.find("=", pos) + 1;
            end = cc.find_last_not_of("0123456789", begin);
            age = stoi(cc.substr(begin, end - begin));
        }
        lifetime = dateToTm(fields["Date"]);
        lifetime.tm_sec += age;
        // mktime(&lifetime);
    } else if(fields.count("Expires")){
        lifetime = dateToTm(fields["Expires"]);
    } else{ 
        time_t now;
        time(&now);
        struct tm *now_tm = gmtime(&now);
        lifetime = *now_tm;
        lifetime.tm_sec += age;
        mktime(&lifetime);
    }

    if(fields.count("ETag")){
        etag = fields["ETag"];
    } else{
        etag = "";
    }
}

// void Parser::compareHead(){
//     string g = reconstruct();
//     // fprintf(stdout, "%s", head_message.c_str());
//     // fprintf(stdout, "%s", g.c_str());
//     fprintf(stdout, "%d\n", head_message.compare(g));
//     for(int i=0; i<(int)head_message.size() && i<(int)g.size(); i++){
//         if(head_message[i] != g[i]){
//             fprintf(stdout, "on the %d th letter, init: %d, gen: %d", i, head_message[i], g[i]);
//         }
//     }
// }