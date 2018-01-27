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
    scroll=0;
    x=0;
    y=0;
    last_action=ACTION_NONE;
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
    if (y < scroll) {
        scroll=y;
    }
    else if (y >= scroll + LINES - 1) {
        scroll=y-LINES+2;
    }
    
    vector<string>::const_iterator iterator;
    int lines_count;

    if (lines_.size() - scroll < (size_t) LINES - 1) {
        lines_count=lines_.size() - scroll;
    }
    else {
        lines_count=LINES - 1;
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

    for (int i=0; i < lines_count; i++) {
        int row=i+scroll;
        move(i, 0);
        
        if (selection_x != -1 && row > begin_y && row <= end_y) {
            int row_length=lines_[row-1].length();
            move (i - 1, row_length);
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
    }
    
    refresh();
    move(y - scroll, x);
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

void FileContentBuffer::delete_line(vector< vector<string> >& clipboard) {
    UndoInfo undo_info;
    undo_info.x=x;
    undo_info.y=y;
    undo_info.type=ACTION_DELETE;
    undo_info.index=clipboard.size();
    undo_history.push_back(undo_info);

    vector<string> paste;
    paste.push_back(lines_[y]);
    paste.push_back("");
    clipboard.push_back(paste);

    if (lines_.size() > 1) {
        lines_.erase(lines_.begin()+y);
    }
    else {
        lines_[0]="";
    }
    
    last_action=ACTION_DELETE_LINE;
}

void FileContentBuffer::insert_char(char character) {
    if (last_action == ACTION_INSERT_CHAR && character != ' ') {
        //undo_history[undo_history.size() - 1].index++;
        small_clipboard[small_clipboard.size() - 1] += character;
    }
    else {
        UndoInfo undo_info;
        undo_info.x=x;
        undo_info.y=y;
        undo_info.type=ACTION_INSERT_CHAR;
        undo_info.index=small_clipboard.size();
        undo_history.push_back(undo_info);        

        string word;
        word += character;
        small_clipboard.push_back(word);
    }

    lines_[y].insert(x++, 1, character);
    
    last_action=ACTION_INSERT_CHAR;
}

int FileContentBuffer::get_x() {
    return x;
}

int FileContentBuffer::get_y() {
    return y;
}

void FileContentBuffer::key_left() {
    if (x-1 >= 0) {
        x--;
    }
    else if (x == 0 && y-1 >= 0) {
        x=(int) lines_[--y].length();
    }
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer::key_right() {
    if (x+1 <= (int) lines_[y].length()) {
        x++;
    }
    else if (y+1 < (int) lines_.size()) {
        x=0;
        y++;
    }
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer::key_up() {
    if (y>0) {
        if (x > (int) lines_[y-1].size()) {
            x=(int) lines_[y-1].length();
        }
        y--;
    }
    else {
        x=0;
    }
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer::key_down() {
    if (y+1 < (int) lines_.size()) {
        if (x > (int) lines_[y+1].size()) {
            x=(int) lines_[y+1].length();
        }
        y++;
    }
    else {
        x=lines_[y].size();
    }
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer::key_backspace() {
    if (x == 0 && y > 0) {
        x=lines_[y-1].size();
        lines_[y-1] += lines_[y];
        lines_.erase(lines_.begin()+y);
        y--;
        
        UndoInfo undo_info;
        undo_info.x=x;
        undo_info.y=y;
        undo_info.type=ACTION_BACKSPACE;
        undo_history.push_back(undo_info);
    }
    else if (x == 0 && y == 0) {
        x=0;
        y=0;
    }
    else {
        lines_[y].erase(x-1, 1);
        x--;
    }
    
    last_action=ACTION_BACKSPACE;
}

void FileContentBuffer::key_delete() {
    if (x < (int) lines_[y].size()) {
        lines_[y].erase(x, 1);
    }
    else if (y < (int) lines_.size() - 1) {
        lines_[y] += lines_[y+1];
        lines_.erase(lines_.begin()+y+1);
        
        UndoInfo undo_info;
        undo_info.x=x;
        undo_info.y=y;
        undo_info.type=ACTION_DELETE;
        undo_history.push_back(undo_info);
    }
    
    last_action=ACTION_DELETE;
}

void FileContentBuffer::key_enter() {
    UndoInfo undo_info;
    undo_info.x=x;
    undo_info.y=y;
    undo_info.type=ACTION_NEW_LINE;
    undo_history.push_back(undo_info);
    
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
    
    last_action=ACTION_NEW_LINE;
}

void FileContentBuffer::word_forward() {
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
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer::word_backwards() {
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
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer:: line_begin() {
    x=0;
    last_action=ACTION_MOVE;
}

void FileContentBuffer:: line_end() {
    x=lines_[y].length();
    last_action=ACTION_MOVE;
}

void FileContentBuffer:: file_begin() {
    x=0;
    y=0;
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer:: file_end() {
    y=lines_.size()-1;   
    x=lines_[y].length();
    
    last_action=ACTION_MOVE;
}

void FileContentBuffer:: set_selection() {
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
    else {
        selection_x=-1;
        selection_y=-1;
    }
}

void FileContentBuffer:: move_selection() {
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
}

void FileContentBuffer:: remove_selection() {
    selection_x=-1;
    selection_y=-1;
}

void FileContentBuffer:: copy_selection(vector< vector<string> >& clipboard) {
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
    
    last_action=ACTION_COPY;
}

void FileContentBuffer:: cut_selection(vector< vector<string> >& clipboard) {
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

    UndoInfo undo_info;
    undo_info.x=begin_x;
    undo_info.y=begin_y;
    undo_info.type=ACTION_CUT;
    undo_info.index=clipboard.size();
    undo_history.push_back(undo_info);
   
    clipboard.push_back(copied);
    
    x=selection_x;
    y=selection_y;
    
    last_action=ACTION_CUT;
}

void FileContentBuffer:: paste_selection(vector< vector<string> >& clipboard) {
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
    
    last_action=ACTION_PASTE;
}

void FileContentBuffer:: undo(vector< vector<string> >& clipboard) {
    if (undo_history.size() == 0) {
        return;
    }
    
    UndoInfo undo_info=undo_history.back();
    undo_history.pop_back();
    
    switch(undo_info.type) {
        case ACTION_INSERT_CHAR:
            lines_[undo_info.y].erase(undo_info.x, small_clipboard[undo_info.index].size());
            x=undo_info.x;
            y=undo_info.y;
            break;
    }
    
    last_action=ACTION_UNDO;
}

void FileContentBuffer:: find_text() {
    last_action=ACTION_MOVE;
}
