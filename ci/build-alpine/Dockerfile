FROM alpine

MAINTAINER Jarle Aase <jgaa@jgaa.com>

RUN apk add --no-cache --update git build-base linux-headers boost-dev bzip2-dev python python-dev cmake &&\
    adduser -D vudnsd && mkdir -p /var/local/src

COPY ./build.sh /var/local/src

RUN cd /var/local/src && ./build.sh &&\
    cd /var/local/src/build && make install &&\
    setcap CAP_NET_BIND_SERVICE=+eip /usr/local/bin/vudnsd &&\
    cp /var/local/src/conf/exampe_zone.txt /etc/vudnsd/zones.conf

USER vudnsd

CMD ["/usr/local/bin/vudnsd", "-z", "-z /etc/vudnsd/zones.conf"]
#
