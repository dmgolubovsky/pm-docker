# Makefile to build ProjectM working with audiofiles. Based on the SDL version.
# Autotools stuff removed where possible - this is a narrow specialized utility


all:
	g++  pmSND.cpp projectM_SND_main.cpp pmSND.hpp \
	-I../projectm/src/libprojectM -I../projectm/src/libprojectM/Renderer/hlslparser/src \
	-Wl,-rpath -Wl,/usr/local/lib \
	-lprojectM -lsndfile -lGL -lSDL2 -lasound -o projectMSND

clean:
	rm -f *.o

install: projectMSND
	cp projectMSND /usr/local/bin

