FROM debian

MAINTAINER Jarle Aase

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -q update &&\
    apt-get -y -q --no-install-recommends upgrade &&\
    apt-get -y -q install git \
        libboost-context1.62.0 libboost-coroutine1.62.0 libboost-program-options1.62.0 \
        libboost-filesystem1.62.0 libboost-regex1.62.0 libboost-chrono1.62.0 \
        libboost-thread1.62.0 libboost-date-time1.62.0 libboost-atomic1.62.0 \
        libicu57 libstdc++6 g++ cmake make libboost-all-dev libcap2-bin &&\
    mkdir -p /var/local/src &&\
    mkdir -p /etc/vudnsd &&\
    cd /var/local/src &&\
    git clone https://github.com/jgaa/vUberdns.git &&\
    cd vUberdns &&\
    rm -fr build &&\
    mkdir build &&\
    cd build &&\
    cmake -DCMAKE_BUILD_TYPE=Release .. &&\
    make -j4 &&\
    cp src/dnsd/vudnsd /usr/bin &&\
    cp ../conf/exampe_zone.txt /etc/vudnsd/zones.conf &&\
    setcap CAP_NET_BIND_SERVICE=+eip /usr/bin/vudnsd &&\
    cd && rm -rf /var/local/src &&\
    apt-get -y purge git automake autoconf build-essential \
        g++ cmake make libboost-all-dev &&\
    apt-get -y -q autoremove &&\
    apt-get -y -q clean

USER nobody
ENV HOME /

EXPOSE 53/udp

CMD ["/bin/sh", "-c", "/usr/bin/vudnsd -z /etc/vudnsd/zones.conf -C -H $HOSTNAME"]

