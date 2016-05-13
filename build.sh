#!/usr/bin/env bash

# This script is a modification of the build for docker2aci
# https://github.com/appc/docker2aci/blob/master/build.sh

set -e

ORG_PATH="github.com/kinvolk"
REPO_PATH="${ORG_PATH}/tcd"

if [ ! -h gopath/src/${REPO_PATH} ]; then
    mkdir -p gopath/src/${ORG_PATH}
    ln -s ../../../.. gopath/src/${REPO_PATH} || exit 255
fi

export GOBIN=${PWD}/bin
export GOPATH=${PWD}/gopath:${PWD}/Godeps/_workspace

eval $(go env)
go get -d # This will be substituted by vendoring

echo "Building tcd..."
# ~/programs/protoc/protoc -I api api/service.proto --go_out=plugins=grpc:api
CGO_ENABLE=0 go build -o $GOBIN/tcd -installsuffix cgo

sudo docker build -t kinvolk/tcd .
