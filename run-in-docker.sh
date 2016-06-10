#!/bin/bash

find_git_branch() {
  # Based on: http://stackoverflow.com/a/13003854/170413
  local branch
  if branch=$(git rev-parse --abbrev-ref HEAD 2> /dev/null); then
    if [[ "$branch" == "HEAD" ]]; then
      branch=''
    fi
    DOCKER_TAG=":${branch//\//-}"
  else
    DOCKER_TAG=""
  fi
}

find_git_branch

echo 1 | sudo tee /proc/sys/net/core/bpf_jit_enable

sudo docker run \
	--rm \
	--privileged \
	--pid=host \
	--net=host \
	-v /etc/machine-id:/etc/machine-id \
	-v /var/run/docker.sock:/var/run/docker.sock \
	-v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket \
	-v /run:/run \
	-v /sys/fs/bpf:/sys/fs/bpf \
	-v $HOME/git/iproute2/examples/bpf/bpf_shared.o:/bpf_shared.o \
	docker.io/$DOCKER_USER/tcd${DOCKER_TAG} \
	/tcd

