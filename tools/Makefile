CFLAGS=-g -Wall

all: mkfs ml apout/apout

mkfs: mkfs.c

ml: ml.c

apout/apout:
	(cd apout; make; rm *.o)

clean:
	rm -f mkfs ml
	(cd apout; make clean)
