FROM ubuntu:18.04
RUN mkdir /var/log/erss
RUN mkdir /proxy
RUN apt-get update
RUN apt-get -y install gcc g++ make
WORKDIR /proxy
ADD . /proxy
RUN make