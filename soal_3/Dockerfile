FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
  libfuse-dev \
  fuse \
  gcc \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY antink.c /app/
RUN gcc -D_FILE_OFFSET_BITS=64 -o antink_fuse antink.c -lfuse

CMD ["./antink_fuse", "/antink_mount"]
