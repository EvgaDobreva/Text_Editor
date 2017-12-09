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
	void insert_char(int& x, int& y, char cursor);
	void new_line(int y);
	void move_key_up(int& y, int& x);
	void move_key_down(int& y, int& x);
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
	ofstream outfile(filename_.c_str());
	vector<string>::const_iterator iterator = lines_.begin();
	outfile << (*iterator).c_str();
	for (++iterator; iterator != lines_.end(); ++iterator) {
	   outfile << "\n";
	   outfile << (*iterator).c_str();
	}
	outfile.close();
}

void FileContentBuffer::delete_line(int y) {
	clrtoeol();
	lines_.erase(lines_.begin()+y);
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

void FileContentBuffer::insert_char(int& x, int& y, char cursor) {
	if (cursor == '\n') {
		if (x == 0) {
			new_line(y-1);
		}
		else if (x == (int) lines_[y].length()) {
			new_line(y);
			x=0;
		}
		else {
			string first = lines_[y].substr(0, x-1);
			string second = lines_[y].substr(x, lines_[y].length() - x);
			new_line(y);
			lines_[y] = first;
			lines_[y+1] = second;
			x=0;
		}
		y++;
	}
	else {
		lines_[y].insert(x++, 1, char(cursor));
	}
}

void FileContentBuffer::new_line(int y) {
	lines_.insert(lines_.begin()+y, "");
}

void FileContentBuffer::move_key_up(int& y, int& x) {
	if (y>0) {
		if (lines_[y-1].size() < lines_[y].size()) {
			x=(int) lines_[y-1].length();
		}
		y--;
	}
}

void FileContentBuffer::move_key_down(int& y, int& x) {
	if (y+1 < (int) lines_.size()) {
		if (lines_[y+1].size() < lines_[y].size()) {
			x=(int) lines_[y+1].length();
		}
		y++;
	}
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
					x=(int) file.lines_[--y].length();
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
				file.move_key_up(y, x);
				break;
			case KEY_DOWN:
				file.move_key_down(y, x);
				break;
			case 4: // Ctrl+A=1, Ctrl+B=2, Ctrl+C=3, Ctrl+D=4, ...
				file.delete_line(y);
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
				if (x == (int) file.lines_[y].size()) {
					file.lines_[y] += file.lines_[y+1];
					file.delete_line(y+1);
				}
				else {
					file.delete_char(x, y, -1);
				}
				break;
			default:
				file.insert_char(x, y, char(cursor));
		}
		file.print();
		move(y, x);
		refresh();
	}
	file.save();
	
	endwin();
	return 0;
}
/*
	1. Change 'q' to Ctrl+q when we exit the editor.
*/
