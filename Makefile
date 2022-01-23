DEBUGARGS=-g -Wall -Wextra -Wshadow -lmingw32 -lSDL2main -lSDL2 -L. -l:liblua54.a
RELEASEARGS=-lmingw32 -lSDL2main -lSDL2 -O2 -L. -l:liblua54.a

SRC=.
OBJ=.
BIN=.

_OBJS = main.o 
OBJS = $(patsubst %,$(OBJ)/%,$(_OBJS))

all: debug

debug: $(OBJS)
	$(CC) $(OBJS) -o $(BIN)/main $(DEBUGARGS) 

release: $(OBJS)
	$(CC) $(OBJS) -o $(BIN)/main $(RELEASEARGS) 

obj/%.o: $(SRC)/%.c
	$(CC) -c $^ -o $@ $ $(DEBUGARGS) 

clean:
	rm $(BIN)/main.exe $(OBJ)/*.o