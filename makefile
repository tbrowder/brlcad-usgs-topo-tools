# clean CMake stuff

FILES  =
FILES += CMakeCache.txt
FILES += cmake_install.cmake
FILES += Makefile


all: clean

clean:
	-rm -rf $(FILES) CMakeFiles



