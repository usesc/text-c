CC=gcc
CFLAGS=`pkg-config --cflags gtk+-3.0 vte-2.91`
LIBS=`pkg-config --libs gtk+-3.0 vte-2.91`
SRC=main.c
OUT=gtkapp

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)
