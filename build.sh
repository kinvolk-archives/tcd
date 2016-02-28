#!/bin/bash -e

# ~/programs/protoc/protoc -I api api/service.proto --go_out=plugins=grpc:api
go build -o ${PWD}/bin/tcd
sudo docker build -t kinvolk/tcd .
