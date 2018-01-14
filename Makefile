editor: editor.cpp filecontentbuffer.o
	g++ -Wall -o $@ $< filecontentbuffer.o -lncurses
	
filecontentbuffer.o: filecontentbuffer.cpp filecontentbuffer.hpp
	g++ -Wall -c $<
