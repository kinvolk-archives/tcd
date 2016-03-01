FROM scratch
MAINTAINER Alban Crequy <alban@kinvolk.io>
ADD bin/tcd /tcd
LABEL works.weave.role=system
EXPOSE 2049
CMD ["/tcd"]
