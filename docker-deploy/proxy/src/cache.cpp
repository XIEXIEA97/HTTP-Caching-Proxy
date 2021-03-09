#include "util.h"
#include "cache.h"

static shared_mutex lock;

// 0: not in cache
// 1: in cache, but expired at EXPIREDTIME
// 2: in cache, requires validation
// 3: in cache, valid
// 4: in cache, but the request states it does not accept cache
CheckResult Cache::checkRequest(const RequestParser &parser){
    shared_lock<shared_mutex> g(lock);
    // check url match
    if(m.count(parser.constructEffectiveUrl()) == 0) 
        return {0};

    auto res = m[parser.constructEffectiveUrl()];

    // check non-cacheable tokens
    auto fields = parser.getFields();
    if(fields.count("Pragma") && fields["Pragma"].find("no-cache") != string::npos) 
        return {4};

    string cc = "";
    if(fields.count("Cache-Control")){
        cc = fields["Cache-Control"];
        if(cc.find("no-cache") != string::npos || cc.find("no-store") != string::npos)
            return {4};
    }

    // check validation
    auto rfields = res.getFields();
    if(rfields.count("Cache-Control") && (rfields["Cache-Control"].find("must-revalidate") != string::npos || rfields["Cache-Control"].find("proxy-revalidate") != string::npos)){
        return {2, res.getEtag()};
    }

    // simplified etag match ...
    if(fields.count("If-None-Match")){
        if(fields["If-None-Match"] != res.getEtag()){
            return {2, fields["If-None-Match"]};
        }
    }
    // If-None-Match ignored ...

    // check freshness
    time_t now;
    time(&now);
    struct tm *now_tm = gmtime(&now);
    struct tm lifetime = res.getLifeTime();
    if(difftime(mktime(&lifetime), mktime(now_tm)) <= 0)
        return {1, tmToDate(lifetime)};

    return {3};
}

// 0: not cacheable because REASON
// 1: cached, expires at EXPIRES
// 2: cached, but requires re-validation
CheckResult Cache::checkResponse(ResponseParser &parser){
    auto fields = parser.getFields();
    int id = parser.getId();
    string cc = "";
    if(fields.count("Cache-Control")){
        cc = fields["Cache-Control"];
        if(cc.find("no-cache") != string::npos)
            return {0, "response states no-cache"};
        if(cc.find("no-store") != string::npos)
            return {0, "response states no-store"};
        if(cc.find("must-revalidate") != string::npos || cc.find("proxy-revalidate") != string::npos){
            if(parser.getEtag().empty())
                return {0, "needs revalidation but no etags"};
            LOG("NOTE Cache-Control: must-revalidate or proxy-revalidate");
        }
    }

    if(!parser.getEtag().empty()){
        LOG("NOTE ETag: " + parser.getEtag());
        return {2};
    }

    // if(parser.getStatusCode() == "301"){
    //     return {0, "it is a 301 response"};
    // }

    if(parser.getStatusCode()[0] == '4'){
        return {0, "it is a 4xx response"};
    }

    struct tm lifetime = parser.getLifeTime();
    return {1, tmToDate(lifetime)};
}

pair<string, string> Cache::_get(const RequestParser &parser){
    shared_lock<shared_mutex> g(lock);
    auto target = m[parser.constructEffectiveUrl()];
    return {target.reconstruct(), target.getStartLine()};
}

pair<string, string> Cache::get(const RequestParser &parser, int id){
    auto ret = _get(parser);
    lock_guard<shared_mutex> g(lock);
    m[parser.constructEffectiveUrl()].setId(id);
    return ret;
}

void Cache::save(string url, const ResponseParser &parser){
    lock_guard<shared_mutex> g(lock);
    if(m.count(url) || (int)m.size() < capacity){
        m[url] = parser;
        // m.insert({url, parser});
        return;
    }

    int mn = INT_MAX;
    auto lru = m.begin();
    for(auto it = m.begin(); it != m.end(); it++){
        if(it->second.getId() < mn){
            mn = it->second.getId();
            lru = it;
        }
    }
    m.erase(lru);

    m[url] = parser;
    // m.insert({url, parser});
}