# HTTP Caching Proxy

## To Run

### run the proxy inside docker

the log would be put under `./docker-deploy/logs/`

	sudo docker-compose -f docker-deploy/docker-compose.yml up

run inside docker and detach so it becomes a daemon

	sudo docker-compose -f docker-deploy/docker-compose.yml up
	
to retach (you'd better not since it is running as daemon and no signal/terminal input would be taken)

	sudo docker attach <container-id>

to stop
 
	sudo docker stop <container-id>

### run the proxy outside docker

first build a directory `/var/log/erss`, the log file would be put here

	sudo mkdir /var/log/erss
	cd docker-deploy/proxy/ ; make ; sudo ./proxy ; cd -

to find the running daemon, and to stop it

	ps -ef | grep proxy
	sudo kill -9 <pid>
	
## Test

two sets of testcases are provided:

### genreal test

	./tests/basic_test.sh

Test the basic functions the proxy should support, which includes get/post/connect request, an invalid request, a post request sending chunk data and a get request that would receive chunk data.

### cache test

	./tests/cap_lru_etag_test.sh

Test the cache LRU policy. The cache size controlled by `CACHESIZE` is set to 2 for test purpose by default which is quite small. When no lifetime directives are provided by the request/response, the heuristic lifetime is 60 seconds after the response is received by the proxy, the default time is controlled by `DEFAULTAGE`.

### others

One could also set up proxy of the browser or for the system and connect to the server's 12345 port, then visit any sites, e.g.

[www.youtube.com](www.youtube.com)

[man7.org](man7.org)

[http://www.artsci.utoronto.ca/futurestudents](http://www.artsci.utoronto.ca/futurestudents)

[http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx](http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx)


## Reference

The socket programming part takes a bit of code from [here](https://beej.us/guide/bgnet/html?)

## some tips

to get some get request header:

	wget -e use_proxy=yes -e http_proxy=localhost:12345 www.google.com -d
	wget -e use_proxy=yes -e https_proxy=localhost:12345 https://www.youtube.com/ -d
