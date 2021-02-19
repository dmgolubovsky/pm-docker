# Build Project M

from ubuntu:20.04

run apt -y update

run env DEBIAN_FRONTEND=noninteractive apt -y install git pkg-config autoconf automake libtool make libgl-dev libsdl2-dev \
                        libglm-dev llvm g++ qt5-default qtdeclarative5-dev

run git clone https://github.com/projectM-visualizer/projectm.git

workdir projectm

run git checkout v3.1.11

run autoreconf --install

run ./configure --help && ./configure --disable-emscripten \
                                      --enable-sdl --disable-jack --disable-pulseaudio --disable-qt --disable-gles

run make

run make install

