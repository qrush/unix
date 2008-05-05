CFLAGS= -g -Wall

disaout: main.o aout.o magic.o opset.o syscalls.o symbols.o
	cc -o disaout main.o aout.o magic.o opset.o syscalls.o symbols.o

clean:
	rm -f disaout *.o
