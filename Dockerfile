FROM ubuntu
MAINTAINER Alban Crequy <alban@kinvolk.io>
ADD bin/tcd /tcd
# ADD bin/tcdclient /tcdclient
LABEL works.weave.role=system
EXPOSE 2049
CMD ["/tcd"]
