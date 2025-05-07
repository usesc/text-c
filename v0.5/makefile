CC=gcc
CFLAGS=`pkg-config --cflags gtk+-3.0 vte-2.91 gtksourceview-4`
LIBS=`pkg-config --libs gtk+-3.0 vte-2.91 gtksourceview-4`
SRC=main2.c
OUT=gtkapp

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)