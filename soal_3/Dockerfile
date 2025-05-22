FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    fuse \
    libfuse-dev \
    pkg-config \
    nano \
    vim \
    && apt-get clean

WORKDIR /app

COPY antink.c .

RUN gcc -Wall antink.c `pkg-config fuse --cflags --libs` -o antink  

RUN mkdir -p /it24_host /antink_mount /var/log

CMD ["./antink", "-f", "/antink_mount"]
