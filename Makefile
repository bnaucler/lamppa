CC := gcc
CFLAGS := -Wall -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf

EXEC := lamppa

# Source files
SRC := lamppa.c

# Default
all:
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

# Make pretty
clean:
	rm -f $(EXEC)
