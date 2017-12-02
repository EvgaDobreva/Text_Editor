#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>

using namespace std;

class FileException {
	
};

class FileContentBuffer {
	string filename_;
public:
	vector<string> lines_;	
	FileContentBuffer(string file);
	void load();
	void print();
	void save();
	void delete_line(int y);
	void delete_char(int x, int y, int direction);
};

FileContentBuffer::FileContentBuffer(string file) {
	filename_=file;
}

void FileContentBuffer::load() {
	ifstream file;
	string line;
	file.open(filename_.c_str());
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
	clear();
	vector<string>::const_iterator iterator;
	for (iterator = lines_.begin(); iterator != lines_.end(); ++iterator) {
	   printw((*iterator).c_str());
		printw("\n");
	}
	refresh();
}

void FileContentBuffer::save() {
	ofstream outfile("out.txt");
	vector<string>::const_iterator iterator;
	for (iterator = lines_.begin(); iterator != lines_.end(); ++iterator) {
	   outfile << (*iterator).c_str();
	   outfile << "\n";
	}
	outfile.close();
}

void FileContentBuffer::delete_line(int y) {
	clrtoeol();
}

void FileContentBuffer::delete_char(int x, int y, int direction) {
	if (direction == 1) {
		lines_[y].erase(x-1, 1);
	}
	if (direction == -1) {
		lines_[y].erase(x, 1);
	}
	print();
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
			case KEY_LEFT:
				if (x-1 >= 0) {
					x--;
				} else if (x == 0 && y-1 >= 0) {
					x=(int) file.lines_[y].length();
					y--;
				}
				break;
			case KEY_RIGHT:
				if (x+1 <= (int) file.lines_[y].length()) {
					x++;
				}
				else if (y+2 < (int) file.lines_.size()) {
					x=0;
					y++;
				}
				break;
			case KEY_UP:
				if (y>0)	y--; break;
			case KEY_DOWN:
				if (y+2 < (int) file.lines_.size()) {
					y++;
				}
				break;
			case 4: // Ctrl+A=1, Ctrl+B=2, Ctrl+C=3, Ctrl+D=4, ...
				file.delete_line(y+1);
				break;
			case KEY_BACKSPACE:
				if (x == 0 && y > 0) {
					x = file.lines_[y-1].size();
					file.lines_[y-1] += file.lines_[y];
					file.delete_line(y);
					y--;
				}
				else if (x == 0 && y == 0) {
					x=0;
					y=0;
				}
				else {
					file.delete_char(x, y, 1);
					x--;
				}
				break;
			case KEY_DC:
				file.delete_char(x, y, -1); break;
			default:
				cout << "UNKNOWN: " << cursor << " ";
		}
		move(y, x);
		refresh();
	}
	file.save();
	
	endwin();
	return 0;
}
