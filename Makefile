# Para usar este Makefile es necesario definir la variable
# de ambiente NSYSTEM con el directorio en donde se encuentra
# la raiz de nSystem.
# Para compilar ingrese make APP=<ejemplo>

# NSYSTEM = direccion de nsystem


LIBNSYS= $(NSYSTEM)/lib/libnSys.a

CFLAGS= -ggdb -I$(NSYSTEM)/include -I$(NSYSTEM)/src
LFLAGS= -ggdb  

all: test

.SUFFIXES:
.SUFFIXES: .o .c .s

test: test.o $(LIBNSYS)
	gcc $(LFLAGS) test.o -o $@ $(LIBNSYS)


clean:
	rm -f *.o *~

cleanall:
	rm -f *.o *~ test