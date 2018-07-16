FROM debian

# This container is meant for building and testing the server,
# not for running it in production (It contains a full development
# environment)

MAINTAINER Jarle Aase <jgaa@jgaa.com>

RUN apt-get -q update &&\
    apt-get -y -q --no-install-recommends upgrade &&\
    apt-get -y -q install git \
    automake autoconf ruby ruby-dev rubygems build-essential \
    zlib1g-dev g++ cmake make libboost-all-dev libcap2-bin &&\
    gem install --no-ri --no-rdoc fpm &&\
    apt-get -y -q autoremove &&\
    apt-get -y -q clean &&\
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
    cd && rm -rf /var/local/src

USER nobody

EXPOSE 53/udp

CMD ["/bin/sh", "-c", "/usr/bin/vudnsd -z /etc/vudnsd/zones.conf -C -H $HOSTNAME"]

