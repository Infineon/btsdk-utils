SRC = u8g_convert.c
OBJ = $(SRC:.c=.o)

convert: $(OBJ)
	$(CC) $(CFLAGS)  $(LDFLAGS) -lm $(OBJ) -o u8g_convert
