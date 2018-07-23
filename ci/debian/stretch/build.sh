
#!/bin/bash

# Compile and prepare a .deb package for distribution
#
# Example:
#   VUDNS_VERSION="2.0.1" DIST_DIR=`pwd`/dist BUILD_DIR=`pwd`/build SRC_DIR=`pwd`/UBDNS  ./UBDNS/scripts/package-deb.sh

if [ -z "$VUDNS_VERSION" ]; then
    VUDNS_VERSION="1.0.0"
    echo "Warning: Missing VUDNS_VERSION variable!"
fi

if [ -z ${DIST_DIR:-} ]; then
    DIST_DIR=`pwd`/dist/linux
fi

if [ -z ${BUILD_DIR:-} ]; then
    BUILD_DIR=`pwd`/build
fi

if [ -z ${SRC_DIR:-} ]; then
    # Just assume we are run from the directory with the script directory
    SRC_DIR=`pwd`/../../../
fi

echo "Building UBDNS for linux into ${DIST_DIR} from ${SRC_DIR}"

rm -rf $DIST_DIR $BUILD_DIR

mkdir -p $DIST_DIR/usr/bin &&\
mkdir -p $DIST_DIR/etc/vudnsd &&\
mkdir -p $BUILD_DIR &&\
cd $BUILD_DIR &&\
cmake DCMAKE_BUILD_TYPE=Release .. &&\
make -j4 &&\
cp src/dnsd/vudnsd $DIST_DIR/usr/bin &&\
cp $SRC_DIR/conf/exampe_zone.txt $DIST_DIR/etc/vudnsd/zones.conf &&\
echo "setcap CAP_NET_BIND_SERVICE=+eip /usr/bin/vudnsd" > $BUILD_DIR/setcap.sh &&\
chmod +x $BUILD_DIR/setcap.sh &&\
cd ${SRC_DIR}
fpm --input-type dir \
    --output-type deb \
    --force \
    --name vuberdns \
    --version ${VUDNS_VERSION} \
    --vendor "The Last Viking LTD" \
    --description "Authoritative DNS server" \
    --depends libboost-context1.62.0 \
    --depends libboost-coroutine1.62.0 \
    --depends libboost-program-options1.62.0 \
    --depends libboost-filesystem1.62.0 \
    --depends libboost-regex1.62.0 \
    --depends libboost-chrono1.62.0 \
    --depends libboost-thread1.62.0 \
    --depends libboost-date-time1.62.0 \
    --depends libboost-atomic1.62.0 \
    --depends libicu57 \
    --depends libcap2-bin \
    --directories /etc/vudnsd \
    --chdir ${DIST_DIR}/ \
    --package ${DIST_NAME}vuberdns-VERSION_ARCH.deb \
    --after-install $BUILD_DIR/setcap.sh &&\
echo "Debian package is available in $PWD"
