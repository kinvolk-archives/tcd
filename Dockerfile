FROM alpine
MAINTAINER Alban Crequy <alban@kinvolk.io>
RUN apk add --update iproute2 && rm -rf /var/cache/apk/*
ADD bin/tcd /tcd
# ADD bin/tcdclient /tcdclient
LABEL works.weave.role=system
EXPOSE 2049
CMD ["/tcd"]
