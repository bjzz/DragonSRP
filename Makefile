CC = g++
CFLAGS = -Wall -ggdb
LIBS = -lssl
BIN = dsrp
OBJ = main.o srpclient.o srpserver.o user.o osslhelp.o osslmathimpl.o osslsha256.o memorylookup.o osslrandom.o ng.o

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ)
	$(CC) $^ -o $@ $(LIBS)

clean::
	rm -f *.o
	rm -f $(BIN)
