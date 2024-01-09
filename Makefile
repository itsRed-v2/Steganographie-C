encode.exe: clean-encode encode.c
	gcc encode.c -o encode -Wall -Llibs -lstb -Ilibs

extract.exe: clean-extract extract.c
	gcc extract.c -o extract -Wall -Llibs -lstb -Ilibs

clean-encode:
	rm -f encode.exe

clean-extract:
	rm -f extract.exe