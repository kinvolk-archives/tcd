#!/bin/bash -e

# ~/programs/protoc/protoc -I api api/service.proto --go_out=plugins=grpc:api
CGO_ENABLED=0 go build -o bin/tcd -installsuffix cgo
sudo docker build -t kinvolk/tcd .
