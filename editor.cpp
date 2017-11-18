#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <list>

using namespace std;

class FileException {
	
};

class FileContentBuffer {
	list<string> lines_;
	string file_;
public:
	FileContentBuffer(string file);
	void load();
	void print();
};

FileContentBuffer::FileContentBuffer(string file) {
	file_=file;
}

void FileContentBuffer::load() {
	ifstream file;
	string line;
	file.open(file_.c_str());
	if (!file.is_open()) {
		throw FileException();
	}
	while (!file.eof()) {
		getline(file, line);
		lines_.push_back(line);
	}
	file.close();
}

void FileContentBuffer::print() {
	list<string>::const_iterator iterator;
	for (iterator = lines_.begin(); iterator != lines_.end(); ++iterator) {
	   printw((*iterator).c_str());
		printw("\n");
	}
	refresh();
}

int main(int argc, char* argv[])
{	
	if (argc < 2) {
		cout << "Need more arguments!" << endl;
		return 1;
	}

	initscr();
	cbreak();
   keypad(stdscr, TRUE);
   noecho();

	FileContentBuffer file(argv[1]);
	file.load();
	file.print();
	
	int x=0;
	int y=0;
	int cursor;
	move(y, x);
	refresh();
	
	while ((cursor = getch()) != 'q') {
		switch(cursor) {
			case KEY_LEFT:	if (x>0) x--; break;
			case KEY_RIGHT: 			x++; break;
			case KEY_UP:	if (y>0)	y--; break;
			case KEY_DOWN: 			y++; break;
			default:
				cout << "UNKNOWN: " << cursor << " ";
		}
		move(y, x);
		refresh();
	}
	
	endwin();
	return 0;
}
/*
	HOMEWORK:
	methods save() and delete() in FileContentBuffer
	save() -> save file after 'q' is pressed and write it in "ofstream" parameter
	delete(x,y) -> delete text in lines_ (lines_[y]) using Backspace or Delete button
	lines_ must be a vector
	
*/
