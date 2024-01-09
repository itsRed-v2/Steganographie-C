all: encode.exe extract.exe

encode.exe: encode.c libs/libstb.a
	gcc encode.c -o encode -Wall -Llibs -lstb -Ilibs

extract.exe: extract.c libs/libstb.a
	gcc extract.c -o extract -Wall -Llibs -lstb -Ilibs

libs/libstb.a: libs/stb.c
	gcc -Wall -c libs/stb.c -o libs/stb.o
	ar ruv libs/libstb.a libs/stb.o