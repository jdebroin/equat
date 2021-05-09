CPP = g++
CPPFLAGS = -c
#debug
CPPFLAGS += -g

OBJECTS = equat.o
LIBS = -lreadline

all: equat
	
equat: $(OBJECTS)
	$(CPP) -o equat $(OBJECTS) $(LIBS)

equat.o: equat.cpp
	$(CPP) $(CPPFLAGS) -o equat.o equat.cpp

install: equat
	cp equat /usr/local/bin

clean:
	rm equat $(OBJECTS)
