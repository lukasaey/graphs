LIBS=-lmingw32 -lSDL2main -lSDL2 -L./lib -l:liblua54.a

DEBUGARGS=-g -Wall -Wextra -Wshadow $(LIBS)
RELEASEARGS=-O2 $(LIBS)

SRC=.
OBJ=obj
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