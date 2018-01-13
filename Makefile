editor: editor.cpp
	g++ -Wall -o $@ $< filecontentbuffer.cpp  -lncurses -lform
