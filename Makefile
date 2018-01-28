editor: editor.cpp textbuffer.o
	g++ -Wall -o $@ $^ -lncurses
	
textbuffer.o: textbuffer.cpp textbuffer.hpp
	g++ -Wall -c $<
