#include "util.h"
#include "parser.h"
#include "proxy.h"
#include "cache.h"

const string response_200 = "HTTP/1.1 200 Connection Established\r\n\r\n";
const string response_400 = "HTTP/1.1 400 Bad Request\r\n\r\n";
const string response_502 = "HTTP/1.1 502 Bad Gateway\r\n\r\n";
Cache cache(CACHESIZE);

void handler(int fd, int id, string ipfrom){
    RequestParser parser(fd, id);
    try{
        parser.fetchHead();
        parser.parseHead();
    } catch(FetchParseException &e){
        LOG("invalid request from " + ipfrom + " @ " + curentDate());
        sendMessage(response_400, fd);
        LOG("Responding " + oneLineResponse(response_400));
        return;
    }

    try{
        Proxy proxy(parser.getHost(), parser.getPort(), fd);
        proxy.init();
        LOG(parser.getStartLine() + " from " + ipfrom + " @ " + curentDate());
        if(parser.getMethod() == "GET"){
            // proxy.handlePost(parser.reconstruct(), id);
            proxy.handleGet(parser);
        } else if(parser.getMethod() == "CONNECT"){
            proxy.handleConnect(id);
        } else if(parser.getMethod() == "POST"){
            parser.fetchContent();
            proxy.handlePost(parser);
        }
    } catch(exception &e){
        LOG("ERROR " + string(e.what()));
    }  
}

int daemon(){
    int sockfd, id = 0;
    struct sockaddr_in address;

    memset(address.sin_zero, 0, sizeof address.sin_zero);
    address.sin_family = AF_INET;
    address.sin_port = htons(MYPORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "main proecess socket failed.\n");
        return -1;
    }

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        fprintf(stderr, "main proecess setsocket failed.\n");
        return -1;
    } 

    if(bind(sockfd, (struct sockaddr*)&address, sizeof address) < 0){
        fprintf(stderr, "main proecess bind failed.\n");
        return -1;
    }

    if(listen(sockfd, BACKLOG) < 0){
        fprintf(stderr, "main proecess listen failed.\n");
        return -1;
    }

    while(1){
        int new_fd;
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd < 0){
            LOG("ERROR accept failed");
            continue;
        }
        string ip(inet_ntoa(((struct sockaddr_in *)&their_addr)->sin_addr));
        thread proxy_handler(handler, new_fd, id++, ip);
        proxy_handler.detach();  
    }

    close(sockfd);
    return 0;
}

int main(int argc, const char * argv[]){
    pid_t cur = getpid();
    pid_t pid = fork();
    if(pid < 0){
        fprintf(stderr, "fork failed.\n");
        return -1;
    }
    if(pid > 0){ //parent
        if(cur != 1) exit(0); // if not inside docker, parent quit
        else while(1);        // else hold the parent
    }
    
    // child
    // dissociate from controlling tty
    pid_t sid = setsid();
    if(sid < 0){
        fprintf(stderr, "setsid failed.\n");
        return -1;
    }
    // clsoe stdin/stderr open them to /dev/null
    int nullfd = open("/dev/null", O_WRONLY);
    if(nullfd < 0){
        fprintf(stderr, "open /dev/null failed.\n");
        return -1;
    }
    if(dup2(nullfd, STDIN_FILENO) < 0){
        fprintf(stderr, "dup2 /dev/null, stdin failed.\n");
        return -1;
    }
    if(dup2(nullfd, STDERR_FILENO) < 0){
        fprintf(stderr, "dup2 /dev/null, stderr failed.\n");
        return -1;
    }
    close(STDIN_FILENO);
    close(STDERR_FILENO);

    // redirect stdout to the log for convenience
    int logfd = open("/var/log/erss/proxy.log", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(logfd < 0){
        fprintf(stdout, "failed to open log file.\n");
        return -1;
    }
    if(dup2(logfd, STDOUT_FILENO) < 0){
        fprintf(stdout, "redirect stdout to log failed.\n");
        return -1;
    }
    
    // change directory to "/"
    if(chdir("/") < 0){
        return -1;
    }
    
    // clear umask
    umask(0);

    // to be non session leader
    pid_t _pid = fork();
    int res;
    if(_pid == 0){
        res = daemon();
        close(logfd);
        close(nullfd);
    }
    return res;
}