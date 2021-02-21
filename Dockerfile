# Build Project M

from ubuntu:20.04

run apt -y update

run env DEBIAN_FRONTEND=noninteractive apt -y install git pkg-config autoconf automake libtool make libgl-dev libsdl2-dev \
                        libglm-dev g++ libsndfile1-dev

add projectm /projectm

workdir /projectm

run autoreconf --install

run ./configure --help && ./configure --disable-emscripten \
                                      --enable-sdl --disable-jack --disable-pulseaudio --disable-qt --enable-gles

run make

run make install

add pmSND /pmSND

workdir /pmSND

run make

run make install
