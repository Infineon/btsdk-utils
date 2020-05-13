#makefile

CC=gcc
CXX=g++

CFLAGS=

OBJ_FILES := wmbt_uart.o \
		wmbt_linux.o \
		btspy_ux.o \
		wmbt.o

LOCAL_INCLUDES ?= . ../common ../../../../dev-kit/btsdk-include


%.o: ../common/%.c
	$(CC) -c -o $@ $< $(CFLAGS)  -I$(LOCAL_INCLUDES)

%.o: ../common/%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)  -I$(LOCAL_INCLUDES)

%.o: ../%.cpp
	$(CC) -c -o $@ $< $(CFLAGS)  -I$(LOCAL_INCLUDES)

wmbt : $(OBJ_FILES)
	$(CC) -o $@ $^  -lstdc++

clean :
	@rm *.o $(OUTPUT_FILE) 2> /dev/null

.PHONY: clean
