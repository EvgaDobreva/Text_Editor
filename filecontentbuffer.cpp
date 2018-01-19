#include "filecontentbuffer.hpp"

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
#include <string.h>

using namespace std;

class FileException {

};

FileContentBuffer::FileContentBuffer(string filename) {
    filename_=filename;
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

void FileContentBuffer::print(int cursor_y, int cursor_x) {
    clear();
    vector<string>::const_iterator iterator;
    int lines_count;

    if (lines_.size() < (size_t) LINES - 1) {
        lines_count=lines_.size();
    }
    else {
        lines_count=LINES - 1;
    }

    int begin_y, begin_x, end_y, end_x;

    if (selection_y > cursor_y || (selection_y == cursor_y && selection_x > cursor_x)) {
        // selection is bigger than cursor
        begin_y=cursor_y;
        begin_x=cursor_x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
        // cursor is bigger than selection
        begin_y=selection_y;
        begin_x=selection_x;
        end_y=cursor_y;
        end_x=cursor_x;
    }

    for (int row=0; row < lines_count; row++) {
        if (selection_x != -1 && row > begin_y && row <= end_y) {
            int row_length=lines_[row-1].length();
            move (row - 1, row_length);
            for (int i=row_length; i < COLS; i++) {
                attron(A_REVERSE);
                printw(" ");
            }
        }

        for (int col = 0; col < (int) lines_[row].length (); col++) {
            if (selection_x != -1 &&
            (row > begin_y || (row == begin_y && col >= begin_x)) &&
            (row < end_y || (row == end_y && col <  end_x))) {
                attron (A_REVERSE);
            }
            else {
                attroff (A_REVERSE);
            }
            printw("%c", lines_[row][col]);
        }
        move (row + 1, 0);
    }
    refresh();
}

void FileContentBuffer::save() {
    ofstream outfile(filename_.c_str());
    vector<string>::const_iterator iterator=lines_.begin();
    outfile << (*iterator).c_str();
    for (++iterator; iterator != lines_.end(); ++iterator) {
        outfile << "\n";
        outfile << (*iterator).c_str();
    }
    outfile.close();
}

void FileContentBuffer::delete_line(int y) {
    if (lines_.size() > 1) {
        lines_.erase(lines_.begin()+y);
    }
    else {
        lines_[0]="";
    }
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

void FileContentBuffer::key_left(int& y, int& x) {
    if (x-1 >= 0) {
        x--;
    }
    else if (x == 0 && y-1 >= 0) {
        x=(int) lines_[--y].length();
    }
}

void FileContentBuffer::key_right(int& y, int& x) {
    if (x+1 <= (int) lines_[y].length()) {
        x++;
    }
    else if (y+1 < (int) lines_.size()) {
        x=0;
        y++;
    }
}

void FileContentBuffer::key_up(int& y, int& x) {
    if (y>0) {
        if (x > (int) lines_[y-1].size()) {
            x=(int) lines_[y-1].length();
        }
        y--;
    }
    else {
        x=0;
    }
}

void FileContentBuffer::key_down(int& y, int& x) {
    if (y+1 < (int) lines_.size()) {
        if (x > (int) lines_[y+1].size()) {
            x=(int) lines_[y+1].length();
        }
        y++;
    }
    else {
        x=lines_[y].size();
    }
}

void FileContentBuffer::key_backspace(int& y, int& x) {
    if (x == 0 && y > 0) {
        x=lines_[y-1].size();
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

void FileContentBuffer::key_delete(int& y, int& x) {
    if (x < (int) lines_[y].size()) {
        delete_char(x, y, -1);
    }
    else if (y < (int) lines_.size() - 1) {
        lines_[y] += lines_[y+1];
        delete_line(y+1);
    }
}

void FileContentBuffer::key_enter(int& y, int& x) {
    if (x == 0) {
        lines_.insert(lines_.begin() + y, "");
        y++;
    }
    else if (x == (int) lines_[y].size()) {
        lines_.insert(lines_.begin() + y + 1, "");
        y++;
        x=0;
    }
    else {
        lines_.insert(lines_.begin() + y + 1, lines_[y].substr(x));
        lines_[y]=lines_[y].substr(0, x);
        y++;
        x=0;
    }
}

void FileContentBuffer::word_forward(int& y, int& x) {
    int next_space_index = lines_[y].find(' ', x);
    if (next_space_index >= 0) {
        while(lines_[y][++next_space_index] == ' ');
            x=next_space_index;
    }
    else if(y == (int) lines_.size() - 1) {
        x=lines_[y].size();
    }
    else { 
        y++;
        x=0;
    }
}

void FileContentBuffer::word_backwards(int& y, int& x) {
    if (x > 0) {
        int next_space_index=lines_[y].rfind(' ', x - 1);
        if (next_space_index >= 0) {
            while(lines_[y][--next_space_index] == ' ');
            x=next_space_index + 1;
        }
        else if(y == 0) {
            x=0;
        }
        else {
            y--;
            x=lines_[y].size();
        }
    }
    else if(y == 0) {
        x=0;
    }
    else {
        y--;
        x=lines_[y].size();
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
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
    else {
        selection_x=-1;
        selection_y=-1;
    }
}

void FileContentBuffer:: move_selection(int y, int x) {
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
}

void FileContentBuffer:: remove_selection() {
    selection_x=-1;
    selection_y=-1;
}

void FileContentBuffer:: copy_selection(int y, int x, vector< vector<string> >& clipboard) {
    if (selection_y == -1 || (selection_y == y && selection_x == x)) {
        return;
    }

    int begin_y, begin_x, end_y, end_x;
    if (selection_y > y || (selection_y == y && selection_x > x)) {
        // selection is bigger than cursor
        begin_y=y;
        begin_x=x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
        // cursor is bigger than selection
        begin_y=selection_y;
        begin_x=selection_x;
        end_y=y;
        end_x=x;
    }
    
    vector<string> copied;

    if (begin_y == end_y) {
        copied.push_back(lines_[y].substr(begin_x, end_x-begin_x));
    }
    else {
        copied.push_back(lines_[begin_y].substr(begin_x));
        for(int i=begin_y+1; i < end_y; i++) {
            copied.push_back(lines_[i]);
        }
        copied.push_back(lines_[end_y].substr(0, end_x));
    }
    clipboard.push_back(copied);
}

void FileContentBuffer:: cut_selection(int& y, int& x, vector< vector<string> >& clipboard) {
    if (selection_y == -1 || (selection_y == y && selection_x == x)) {
        return;
    }

    int begin_y, begin_x, end_y, end_x;
    if (selection_y > y || (selection_y == y && selection_x > x)) {
        // selection is bigger than cursor
        begin_y=y;
        begin_x=x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
        // cursor is bigger than selection
        begin_y=selection_y;
        begin_x=selection_x;
        end_y=y;
        end_x=x;
    }
    
    vector<string> copied;

    if (begin_y == end_y) {
        copied.push_back(lines_[y].substr(begin_x, end_x-begin_x));
        lines_[y].erase(begin_x, end_x-begin_x);
    }
    else {
        copied.push_back(lines_[begin_y].substr(begin_x));
        lines_[begin_y].erase(begin_x);
        for(int i=begin_y+1; i < end_y; i++) {
            copied.push_back(lines_[begin_y+1]);
            lines_.erase(lines_.begin()+begin_y+1);
        }
        copied.push_back(lines_[begin_y+1].substr(0, end_x));
        lines_[begin_y] += lines_[begin_y+1].substr(end_x);
        lines_.erase(lines_.begin()+begin_y+1);
    }

    clipboard.push_back(copied);
    
    x=selection_x;
    y=selection_y;
}

void FileContentBuffer:: paste_selection(int& y, int& x, vector< vector<string> >& clipboard) {
    if (clipboard.size() == 0) {
        return;
    }
    vector<string> pasted=clipboard.back();
    string text_after_x=lines_[y].substr(x);
    lines_[y].erase(x);
    lines_[y] += pasted[0];
    y++;
    for (size_t i=1; i < pasted.size(); i++) {
        lines_.insert(lines_.begin()+y, pasted[i]);
        y++;
    }
    y--;
    x=lines_[y].length();
    lines_[y] += text_after_x;
}

void FileContentBuffer:: find_text() {
    
}
