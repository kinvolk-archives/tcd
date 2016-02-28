#!/bin/bash -e

go build -o ${PWD}/bin/tcd
sudo docker build -t kinvolk/tcd .
