FROM alpine:3.11

WORKDIR /tmp

RUN apk --no-cache add python ffmpeg tzdata bash \
&& apk --no-cache add --virtual=builddeps autoconf automake libtool git ffmpeg-dev wget tar build-base \
&& wget http://prdownloads.sourceforge.net/argtable/argtable2-13.tar.gz \
&& tar xzf argtable2-13.tar.gz \
&& cd argtable2-13/ && ./configure && make && make install \
&& cd /tmp && git clone https://github.com/erikkaashoek/Comskip \
&& cd Comskip && ./autogen.sh && ./configure && make && make install \
&& apk del builddeps \
&& rm -rf /var/cache/apk/* /tmp/* /tmp/.[!.]*

ENTRYPOINT [ "comskip"]
CMD [comskip]
