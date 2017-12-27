#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>
#include <iterator>
#include <string>
#include <stdexcept>
#include <termios.h>

using namespace std;

class FileException {
	
};

class FileContentBuffer {
	string filename_;
	vector<string> lines_;
public:	
	FileContentBuffer(string file);
	void load();
	void print();
	void save();
	void delete_line(int y);
	void delete_char(int x, int y, int direction);
	void insert_char(int& x, int& y, char cursor);
	void new_line(int y);
	void move_key_left(int& y, int& x);
	void move_key_right(int& y, int& x);
	void move_key_up(int& y, int& x);
	void move_key_down(int& y, int& x);
	void move_key_backspace(int& y, int& x);
	void move_key_delete(int& y, int& x);
	void word_forward(int& y, int& x);
	void word_backwards(int& y, int& x);
	void line_forward(int& y, int& x);
	void line_backwards(int& y, int& x);
	void file_forward(int& y, int& x);
	void file_backwards(int& y, int& x);
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
	int counter=0;
	for (iterator = lines_.begin(); iterator != lines_.end(); ++iterator) {
	   printw((*iterator).c_str());
		printw("\n");
		counter++;
		if (counter == LINES-1) {
			break;
		}
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
			string first=lines_[y].substr(0, x-1);
			string second=lines_[y].substr(x, lines_[y].length() - x);
			new_line(y);
			lines_[y]=first;
			lines_[y+1]=second;
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

void FileContentBuffer::move_key_left(int& y, int& x) {
	if (x-1 >= 0) {
		x--;
	}
	else if (x == 0 && y-1 >= 0) {
		x=(int) lines_[--y].length();
	}
}

void FileContentBuffer::move_key_right(int& y, int& x) {
	if (x+1 <= (int) lines_[y].length()) {
		x++;
	}
	else if (y+2 < (int) lines_.size()) {
		x=0;
		y++;
	}
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

void FileContentBuffer::move_key_backspace(int& y, int& x) {
	if (x == 0 && y > 0) {
		x = lines_[y-1].size();
		lines_[y-1] += lines_[y];
		delete_line(y);
		y--;
	}
	else if (x == 0 && y == 0) {
		x=0;
		y=0;
	}
	else {
		delete_char(x, y, 1);
		x--;
	}
}

void FileContentBuffer::move_key_delete(int& y, int& x) {
	if (x == (int) lines_[y].size()) {
		lines_[y] += lines_[y+1];
		delete_line(y+1);
	}
	else {
		delete_char(x, y, -1);
	}
}

void FileContentBuffer::word_forward(int& y, int& x) {
	int next_space_index = lines_[y].find(' ', x);
	if (next_space_index == -1 && y+1 < (int) lines_.size()) {
		y++;
	}
	x=next_space_index + 1;
}

void FileContentBuffer::word_backwards(int& y, int& x) {
	int next_space_index=lines_[y].rfind(' ', x);
	if (next_space_index == -1 && y > 0) {
		y--;
		x=0;
	}
	if (next_space_index - 1 < 0) {
		x=0;
	}
	else {
		x=next_space_index - 1;
	}
}

void FileContentBuffer:: line_forward(int& y, int& x) {
   
}

void FileContentBuffer:: line_backwards(int& y, int& x) {
   
}

void FileContentBuffer:: file_forward(int& y, int& x) {
   
}

void FileContentBuffer:: file_backwards(int& y, int& x) {
   
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
   
   struct termios options;
   tcgetattr(2, &options);
   options.c_iflag &= ~(IXON|IXOFF);
   tcsetattr(2, TCSAFLUSH, &options);

	FileContentBuffer file(argv[1]);
	file.load();
	file.print();
	
	int x=0;
	int y=0;
	int cursor;
	move(y, x);
	refresh();
	
	while ((cursor = getch()) != 17) {
		const char* status="";
		switch(cursor) {		
			case KEY_LEFT: // left
				file.move_key_left(y, x);
				status="Left";
				break;
			case KEY_RIGHT: // right
				file.move_key_right(y, x);
				status="Right";
				break;
			case KEY_UP: // up
				file.move_key_up(y, x);
				status="Up";
				break;
			case KEY_DOWN: // down
				file.move_key_down(y, x);
				status="Down";
				break;
			case 4: // Ctrl+D=4
				file.delete_line(y);
				status="Line deleted";
				break;
			case KEY_BACKSPACE: // backspace
				file.move_key_backspace(y, x);
				status="Character deleted using BACKSPACE";
				break;
			case KEY_DC: // delete
				file.move_key_delete(y, x);
				status="Character deleted using DELETE";
				break;
			case 19: // Ctrl+S
				file.save();
				status="File Saved";
				break;
			case 23: // Ctrl+W
				file.word_forward(y, x);
				status="Moved forward by word";
				break;
			case 2: // Ctrl+B
				file.word_backwards(y, x);
				status="Moved backwards by word";
				break;
			case 1: // Ctrl+A
			   file.line_forward(y, x);
			   status="Forward on the line";
			   break;
			case 5: // Ctrl+E
				file.line_backwards(y, x);
				status="Backwards on the line";
				break;
			/*case :
				file.file_forward(y, x);
				status="Beginning of the file";
				break;
			case :
				file.file_backwards(y, x);
				status="End of file";
				break;
			*/
			default:
				if (cursor >= ' ' && cursor <= '~') {
					file.insert_char(x, y, char(cursor));
				}
		}
		file.print();
		move(LINES-1, 0);

		attron(A_REVERSE);
		printw("Ln:%d, Col:%d  %s", y+1, x+1, status);
		for(int i=0; i < COLS;i++) {
			printw(" ");
		}
		attroff(A_REVERSE);

		move(y, x);
		refresh();
	}
	
	endwin();
	return 0;
}
/*
	1. преместване на курсора в началото и края на ред (Control+a - началото на реда Control+e - края на реда) - 100%
	2. преместване на курсора в началото и края на файла (Control+shift+a и Control+shift+e) - 100% да бъде направено
	3. как се прави селектиране на текст (shift+arrows) за наляво и надясно KEY_SLEFT, KEY_SRIGHT ...

   1. Moving word by word with control+keys. TODO: FIX moving forward and backward when there are multiple spaces.
*/
