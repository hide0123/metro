FROM debian:latest

RUN apt-get update && apt-get install -y clang make

WORKDIR /metro

COPY include/ include/
COPY src/     src/
COPY Makefile .

RUN make install -j

WORKDIR /
