CFLAGS=-g -Wall

all: mkfs ml apout/apout

mkfs: mkfs.c

ml: ml.c

apout/apout:
	(cd apout; make; rm *.o)

clean:
	rm mkfs ml
	(cd apout; make clean)
