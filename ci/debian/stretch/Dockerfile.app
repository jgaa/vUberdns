FROM debian:stretch

MAINTAINER Jarle Aase

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -q update &&\
    apt-get -y -q --no-install-recommends upgrade

COPY vudnsd.deb /root/vudnsd.deb
RUN apt install -y /root/vudnsd.deb && rm /root/vudnsd.deb

USER nobody
ENV HOME /

EXPOSE 53/udp

CMD ["/bin/sh", "-c", "/usr/bin/vudnsd -z /etc/vudnsd/zones.conf -C -H $HOSTNAME"]
