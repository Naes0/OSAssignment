CC = gcc
CFLAGS = -Wall -g -std=gnu99 -lrt -lpthread -pedantic
OBJ = sds.o
EXEC = sds

$(EXEC) : $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) -lrt -lpthread

sds.o : sds.c sds.h
	$(CC) -c sds.c $(CFLAGS) -lrt -lpthread

clean:
	rm -f $(EXEC) $(OBJ)
