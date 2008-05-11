
all :
	(cd tools; make)
	(cd build; make install)

clean :
	(cd build; make clean)

clobber : clean
	(cd tools; make clean)
	(cd build; make clobber)

