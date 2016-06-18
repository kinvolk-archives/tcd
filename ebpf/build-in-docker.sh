#!/bin/sh
sudo docker run -v $PWD:/src alpine:edge /bin/sh -c 'cd /src && apk add --update clang clang-libs clang-dev linux-headers libc-dev make && make'
