# Build Project M

from ubuntu:20.04

run apt -y update

run env DEBIAN_FRONTEND=noninteractive apt -y install git pkg-config autoconf automake libtool make libgl-dev libsdl2-dev \
                        libglm-dev g++ libsndfile1-dev

run git clone https://github.com/projectM-visualizer/projectm.git

workdir projectm

run git checkout v3.1.12

run autoreconf --install

run ./configure --help && ./configure --disable-emscripten \
                                      --enable-sdl --disable-jack --disable-pulseaudio --disable-qt --enable-gles

run make

run make install

