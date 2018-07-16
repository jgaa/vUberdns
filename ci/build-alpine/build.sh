#!/bin/sh

# cmake -DBoost_USE_STATIC_LIBS=ON -DCMAKE_BUILD_TYPE=Release  .. &&\

git clone https://github.com/jgaa/vUberdns.git && \
cd vUberdns &&\
rm -fr build && \
mkdir build && \
cd build && \
cmake -DCMAKE_BUILD_TYPE=Release  .. &&\
make -j4 &&\
cd ..
