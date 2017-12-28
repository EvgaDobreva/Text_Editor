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
	int selection_x;
	int selection_y;
public:	
	FileContentBuffer(string file);
	void load();
	void print(int& cursor_y, int& cursor_x);
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
	void line_begin(int& x);
	void line_end(int& y, int& x);
	void file_begin(int& y, int& x);
	void file_end(int& y, int& x);
	void set_selection(int y, int x);
};

FileContentBuffer::FileContentBuffer(string file) {
	filename_=file;
	selection_x=-1;
	selection_y=-1;
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

void FileContentBuffer::print(int& cursor_y, int& cursor_x) {
  clear();
  vector<string>::const_iterator iterator;
  int lines_count;

  if (lines_.size() < (size_t) LINES - 1) {
    lines_count=lines_.size();
  }
  else {
    lines_count=LINES - 1;
  }

  int y1, x1, y2, x2;

  if (selection_y > cursor_y || (selection_y == cursor_y && selection_x > cursor_x))
    {
      // selection is bigger than cursor
      y1 = cursor_y;
      x1 = cursor_x;
      y2 = selection_y;
      x2 = selection_x;
    }
  else
    {
      // cursor is bigger than selection
      y1 = selection_y;
      x1 = selection_x;
      y2 = cursor_y;
      x2 = cursor_x;
    }

  for (int row=0; row < lines_count; row++) {
    for(int column=0; column < (int) lines_[row].length(); column++) {
      attroff(A_REVERSE);

      if (selection_x != -1 &&
          (row > y1 || (row == y1 && column >= x1)) &&
          (row < y2 || (row == y2 && column < x2)))
        {
          attron(A_REVERSE);
        }

      printw("%c", lines_[row][column]);
    }
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
	print(y, x);
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

void FileContentBuffer:: line_begin(int& x) {
   x=0;
}

void FileContentBuffer:: line_end(int& y, int& x) {
   x=lines_[y].length();
}

void FileContentBuffer:: file_begin(int& y, int& x) {
   x=0;
	y=0;
}

void FileContentBuffer:: file_end(int& y, int& x) {
	y=lines_.size()-1;   
	x=lines_[y].length();
}

void FileContentBuffer:: set_selection(int y, int x) {
	selection_x=x;
	selection_y=y;
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
	
	int x=0;
	int y=0;

	FileContentBuffer file(argv[1]);
	file.load();
	file.print(y, x);
	
	int cursor;
	move(y, x);
	refresh();
		
	while ((cursor = getch()) != 17) {
		ostringstream status;
		switch(cursor) {		
			case KEY_LEFT: // left
				file.move_key_left(y, x);
				status << "Left";
				break;
			case KEY_RIGHT: // right
				file.move_key_right(y, x);
				status << "Right";
				break;
			case KEY_UP: // up
				file.move_key_up(y, x);
				status << "Up";
				break;
			case KEY_DOWN: // down
				file.move_key_down(y, x);
				status << "Down";
				break;
			case 4: // Ctrl+D=4
				file.delete_line(y);
				status << "Line deleted";
				break;
			case KEY_BACKSPACE: // backspace
				file.move_key_backspace(y, x);
				status << "Deleting using Backspace";
				break;
			case KEY_DC: // delete
				file.move_key_delete(y, x);
				status << "Deleting using Delete";
				break;
			case 19: // Ctrl+S
				file.save();
				status << "File Saved";
				break;
			case 23: // Ctrl+W
				file.word_forward(y, x);
				status << "Moved forward by word";
				break;
			case 2: // Ctrl+B
				file.word_backwards(y, x);
				status << "Moved backwards by word";
				break;
			case 1: // Ctrl+A
			   file.line_begin(x);
			   status << "Beginning of the line";
			   break;
			case 5: // Ctrl+E
				file.line_end(y, x);
				status << "End of the line";
				break;
			case '\0':
				file.set_selection(y, x);
				status << "Set selection";
				break;
			case 27: // ESC (alt was pressed along with another key)
				cursor=getch();
				switch(cursor) {
					case 'a':
						file.file_begin(y, x);
						status << "Beginning of the file";
						break;
					case 'e':
						file.file_end(y, x);
						status << "End of file";
						break;
					default:
						status << "Unknown command: '" << (char) cursor << '\'';
				}
				break;
			default:
				if (cursor >= ' ' && cursor <= '~') {
					file.insert_char(x, y, char(cursor));
				}
				else {
					status << "Unknown command: '";
					if (cursor == 0) {
					   status << "^@";
					}
					else {
						status << (char) cursor;
					}
					status << '\'';
				}
		}
		file.print(y, x);
		move(LINES-1, 0);

		attron(A_REVERSE);
		printw("Ln:%d, Col:%d  %s", y+1, x+1, status.str().c_str());
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
   TODO:
   1. Да се оправи Backspace
   2. Местене на курсора със стрелките
   3. Enter команда за добавяне на нови редове
   4. Преминаване на следващата дума - не работи на последния ред, ако има текст
   5. Delete при края на файл - забива
   6. Местене дума по дума - да се поправи, когато има повече спейса.

	7. преместване на курсора в началото и края на ред (Control+a - началото на реда Control+e - края на реда)  Готово!
	8. преместване на курсора в началото и края на файла (Control+shift+a и Control+shift+e - май не е възможно през терминал, направени са с Alt)  Готово!
	9. как се прави селектиране на текст (shift+arrows) за наляво и надясно KEY_SLEFT, KEY_SRIGHT ...
*/
