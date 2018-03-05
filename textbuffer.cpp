#include "textbuffer.hpp"

#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <sstream>
#include <termios.h>
#include <string.h>

using namespace std;

const char* DATA_TYPES[]={"auto", "bool", "char", "char16_t", "char32_t", "const", "double", "explicit", "export", "extern", "float", "inline", "int", "long", "mutable", "register", "short", "signed", "static", "unsigned", "void", "volatile", "wchar_t"};
size_t DATA_TYPES_LEN=23;
const char* KEYWORDS[]={"alignas", "alignof", "and", "and_eq", "asm", "bitand", "bitor", "break", "case", "catch", "class", "compl", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do", "dynamic_cast", "else", "enum", "false", "for", "friend", "goto", "if", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "reinterpret_cast", "return", "sizeof", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "using", "virtual", "while", "xor", "xor_eq", "override", "final", "include"};
size_t KEYWORDS_LEN=63;

class FileException {

};

TextBuffer::TextBuffer() {
    selection_x=-1;
    selection_y=-1;
    scroll=0;
    x=0;
    y=0;
    last_action=ACTION_NONE;
    undo_count=0;
    last_action_modified=false;
}

void TextBuffer::load_file(string filename) {
    ifstream file(filename.c_str());
    if (!file.is_open()) {
        throw FileException();
    }
    string line;
    while (!file.eof()) {
        getline(file, line);
        lines_.push_back(line);
    }
    file.close();
}

void TextBuffer::save(string filename) {
    ofstream outfile(filename.c_str());
    vector<string>::const_iterator iterator=lines_.begin();
    outfile << (*iterator).c_str();
    for (++iterator; iterator != lines_.end(); ++iterator) {
        outfile << "\n";
        outfile << (*iterator).c_str();
    }
    outfile.close();
}

void TextBuffer::init_empty() {
    string empty="";
    lines_.push_back(empty);
}

void TextBuffer::clear() {
    lines_.clear();
}

void TextBuffer::insert_lines(vector<string> new_lines) {
    string text_after_x=lines_[y].substr(x);
    lines_[y].erase(x);
    lines_[y] += new_lines[0];
    y++;
    for (size_t i=1; i < new_lines.size(); i++) {
        lines_.insert(lines_.begin()+y, new_lines[i]);
        y++;
    }
    y--;
    x=lines_[y].length();
    lines_[y] += text_after_x;
}

void TextBuffer::update(int buffer_x, int width, vector< vector<string> > clipboard, TextBuffer* debug_buffer) {
    use_default_colors();
    start_color();

    if (undo_count > 0 && last_action_modified && last_action != ACTION_UNDO && last_action != ACTION_REDO) {
        undo_history.erase(undo_history.end() - undo_count - 1, undo_history.end() - 1);
        undo_count=0;
    }

    if (debug_buffer) {
        debug_buffer->clear();

        string line = "|last_action_modified: ";
        line += last_action_modified ? "true" : "false";
        debug_buffer->insert_line(line);

        debug_buffer->insert_line("|clipboard:");

        for(size_t i=0; i < clipboard.size(); i++) {
            stringstream line;
            line << i << ": " << endl;
            debug_buffer->insert_line(line.str());
            for(size_t j=0; j < clipboard[i].size(); j++) {
                stringstream line;
                line << "  \"" << clipboard[i][j] << '"' << endl;
                debug_buffer->insert_line(line.str());
            }
        }

        debug_buffer->insert_line("|undo_history:");
        for(size_t i=0; i < undo_history.size(); i++) {
            UndoInfo undo_info=undo_history[i];
            stringstream line;
            line << "  "
                 << "x=" << undo_info.x << " "
                 << "y=" << undo_info.y << " "
                 << "type=" << undo_info.type << " "
                 << "index=" << undo_info.index << " "
                 << "data=\"" << undo_info.data << '"';
            debug_buffer->insert_line(line.str());
        }
    }

    if (width == 0) {
        width=COLS-buffer_x;
    }
    else if (width > COLS-buffer_x) {
        width=COLS-buffer_x;
    }

    last_action_modified=false;

    if (y < scroll) {
        scroll=y;
    }
    else if (y >= scroll + LINES - 1) {
        scroll=y-LINES+2;
    }

    int lines_count;

    if (lines_.size() - scroll < (size_t) LINES - 1) {
        lines_count=lines_.size() - scroll;
    }
    else {
        lines_count=LINES - 1;
    }

    int begin_y, begin_x, end_y, end_x;

    if (selection_y > y || (selection_y == y && selection_x > x)) {
        begin_y=y;
        begin_x=x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
        begin_y=selection_y;
        begin_x=selection_x;
        end_y=y;
        end_x=x;
    }

    for (int line_index=0; line_index < lines_count; line_index++) {
        int row=line_index+scroll;
        move(line_index, buffer_x);

        string line = lines_[row];
        int line_width = (int) line.length() > width ? width : line.length();

        for (int col = 0; col < line_width;) {
            char c=line[col];
            string word;
            int word_color = 0;
            if (c == '"') {
                size_t end;
                for (end=col+1; end < (size_t) line_width; end++) {
                    if (line[end] == '"' && line[end-1] != '\\') {
                        break;
                    }
                }
                word_color=4;
                word = line.substr(col, end-col+1);
            }
            else if (c == '\'') {
                size_t end;
                for (end=col+1; end < (size_t) line_width; end++) {
                    if (line[end] == '\'' && line[end-1] != '\\') {
                        break;
                    }
                }
                word_color=4;
                word = line.substr(col, end-col+1);
            }
            else if (c == '/' && col + 1 != (int) line.length() && line[col+1] == '/') {
                word_color=5;
                word = line.substr(col, line_width-col);
            }
            else if (c == '/' && col + 1 != (int) line.length() && line[col+1] == '*') {
                size_t end;
                for (end=col+2; end < (size_t) line_width - 1; end++) {
                    if (line[end] == '*' && line[end+1] == '/') {
                        break;
                    }
                }
                word_color=5;
                word = line.substr(col, end-col+2);
            }
            else if (c >= '0' && c <= '9')
              {
                size_t end;
                for (end=col+1; end < (size_t) line_width; end++) {
                    c=line[end];
                    if ((c < '0' || c > '9') && c != '.') {
                        break;
                    }
                }
                word_color=3;
                word=line.substr(col, end-col);
              }
            else if (c >= 'a' && c <= 'z') {
                size_t end;
                for (end=col+1; end < (size_t) line.length(); end++) {
                    c=line[end];
                    if ((c < 'a' || c > 'z') &&
                        (c < '0' || c > '9') && c != '_') {
                        break;
                    }
                }

                word=line.substr(col, end-col);

                bool is_keyword=false;
                for (size_t i=0; i < DATA_TYPES_LEN; i++) {
                    if (word == DATA_TYPES[i]) {
                        is_keyword=true;
                        word_color=1;
                        break;
                    }
                }
                if(!is_keyword) {
                    for (size_t i=0; i < KEYWORDS_LEN; i++) {
                        if (word == KEYWORDS[i]) {
                            is_keyword=true;
                            word_color=2;
                            break;
                        }
                    }
                }

                if (col + word.length() > (size_t) line_width) {
                    word = word.substr(0, line_width - col);
                }
            }
            else {
                word+=c;
            }

            attron(COLOR_PAIR(word_color));
            if (selection_x != -1) {
                if (row > begin_y && row < end_y) {
                    attron (A_REVERSE);
                    printw("%s", word.c_str());
                    attroff (A_REVERSE);
                }
                else if (row == begin_y && row == end_y) {
                    int end = col + word.length();
                    if (col >= end_x || end <= begin_x) {
                        printw("%s", word.c_str());
                    }
                    else if (col < begin_x && end > end_x) {
                        size_t left_split = begin_x - col;
                        size_t right_split = end_x - col;
                        printw("%s", word.substr(0, left_split).c_str());
                        attron (A_REVERSE);
                        printw("%s", word.substr(left_split, right_split - left_split).c_str());
                        attroff (A_REVERSE);
                        printw("%s", word.substr(right_split).c_str());
                    }
                    else if (col < begin_x) {
                        size_t split = begin_x - col;
                        printw("%s", word.substr(0, split).c_str());
                        attron (A_REVERSE);
                        printw("%s", word.substr(split).c_str());
                        attroff (A_REVERSE);
                    }
                    else if (end > end_x) {
                        size_t split = end_x - col;
                        attron (A_REVERSE);
                        printw("%s", word.substr(0, split).c_str());
                        attroff (A_REVERSE);
                        printw("%s", word.substr(split).c_str());
                    }
                    else {
                        attron (A_REVERSE);
                        printw("%s", word.c_str());
                        attroff (A_REVERSE);
                    }
                }
                else if (row == begin_y) {
                    if ((int) (col + word.length()) <= begin_x) {
                        printw("%s", word.c_str());
                    }
                    else if (col >= begin_x) {
                        attron (A_REVERSE);
                        printw("%s", word.c_str());
                        attroff (A_REVERSE);
                    }
                    else {
                        size_t split = begin_x - col;
                        printw("%s", word.substr(0, split).c_str());
                        attron (A_REVERSE);
                        printw("%s", word.substr(split).c_str());
                        attroff (A_REVERSE);
                    }
                }
                else if (row == end_y) {
                    if (col >= end_x) {
                        printw("%s", word.c_str());
                    }
                    else if ((int) (col + word.length()) <= end_x) {
                        attron (A_REVERSE);
                        printw("%s", word.c_str());
                        attroff (A_REVERSE);
                    }
                    else {
                        size_t split = end_x - col;
                        attron (A_REVERSE);
                        printw("%s", word.substr(0, split).c_str());
                        attroff (A_REVERSE);
                        printw("%s", word.substr(split).c_str());
                    }
                }
                else {
                    printw("%s", word.c_str());
                }
            }
            else {
                printw("%s", word.c_str());
            }
            attroff(COLOR_PAIR(word_color));

            col+=word.length();
        }
    }
    refresh();
}

void TextBuffer::activate_buffer(int buffer_x) {
    move(y - scroll, x + buffer_x);
}

void TextBuffer::insert_line(string line) {
    lines_.push_back(line);
}

void TextBuffer::delete_line(vector< vector<string> >& clipboard) {
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
    last_action_modified=true;
}

void TextBuffer::insert_char(char character) {
    if (last_action == ACTION_INSERT_CHAR) {
        if (character != ' ' && undo_history.back().data[undo_history.back().data.length() - 1] == ' ') {
            UndoInfo undo_info;
            undo_info.x=x;
            undo_info.y=y;
            undo_info.type=ACTION_INSERT_CHAR;
            undo_info.index=0;
            undo_info.data += character;
            undo_history.push_back(undo_info);
        }
        else {
            undo_history.back().data += character;
        }
    }
    else {
        UndoInfo undo_info;
        undo_info.x=x;
        undo_info.y=y;
        undo_info.type=ACTION_INSERT_CHAR;
        undo_info.index=0;
        undo_info.data += character;
        undo_history.push_back(undo_info);
    }

    lines_[y].insert(x++, 1, character);

    last_action=ACTION_INSERT_CHAR;
    last_action_modified=true;
}

void TextBuffer::key_backspace() {
    if (x == 0) {
        if (y > 0) {
            x=lines_[y-1].size();
            lines_[y-1] += lines_[y];
            lines_.erase(lines_.begin()+y);
            y--;

            UndoInfo undo_info;
            undo_info.x=x;
            undo_info.y=y;
            undo_info.type=ACTION_MERGE_LINE;
            undo_info.index=0;
            undo_history.push_back(undo_info);
        }
    }
    else {
        x--;

        if (last_action == ACTION_BACKSPACE &&
            ((lines_[y][x] != ' ' && undo_history.back().data[0] != ' ') ||
             (lines_[y][x] == ' ' && undo_history.back().data[0] == ' '))) {
            undo_history.back().data.insert(undo_history.back().data.begin(), lines_[y][x]);
            undo_history.back().x--;
        }
        else {
            UndoInfo undo_info;
            undo_info.x=x;
            undo_info.y=y;
            undo_info.type=ACTION_BACKSPACE;
            undo_info.index=0;
            undo_info.data=lines_[y][x];
            undo_history.push_back(undo_info);
        }

        lines_[y].erase(x, 1);
    }

    last_action=ACTION_BACKSPACE;
    last_action_modified=true;
}

void TextBuffer::key_delete() {
    if (x < (int) lines_[y].size()) {
        if (last_action == ACTION_DELETE) {
            undo_history.back().data += lines_[y][x];
        }
        else {
            UndoInfo undo_info;
            undo_info.x=x;
            undo_info.y=y;
            undo_info.type=ACTION_DELETE;
            undo_info.index=0;
            undo_info.data=lines_[y][x];
            undo_history.push_back(undo_info);
        }

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
    last_action_modified=true;
}

void TextBuffer::key_enter() {
    UndoInfo undo_info;
    undo_info.x=x;
    undo_info.y=y;
    undo_info.type=ACTION_SPLIT_LINE;
    undo_info.index=0;
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

    last_action=ACTION_SPLIT_LINE;
    last_action_modified=true;
}

int TextBuffer::get_x() {
    return x;
}

int TextBuffer::get_y() {
    return y;
}

void TextBuffer::key_left() {
    if (x-1 >= 0) {
        x--;
    }
    else if (x == 0 && y-1 >= 0) {
        x=(int) lines_[--y].length();
    }

    last_action=ACTION_MOVE;
}

void TextBuffer::key_right() {
    if (x+1 <= (int) lines_[y].length()) {
        x++;
    }
    else if (y+1 < (int) lines_.size()) {
        x=0;
        y++;
    }

    last_action=ACTION_MOVE;
}

void TextBuffer::key_up() {
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

void TextBuffer::key_down() {
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

void TextBuffer::word_forward() {
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

void TextBuffer::word_backwards() {
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

void TextBuffer:: line_begin() {
    x=0;
    last_action=ACTION_MOVE;
}

void TextBuffer:: line_end() {
    x=lines_[y].length();
    last_action=ACTION_MOVE;
}

void TextBuffer:: file_begin() {
    x=0;
    y=0;

    last_action=ACTION_MOVE;
}

void TextBuffer:: file_end() {
    y=lines_.size()-1;
    x=lines_[y].length();

    last_action=ACTION_MOVE;
}

void TextBuffer:: set_selection() {
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
    else {
        selection_x=-1;
        selection_y=-1;
    }
}

void TextBuffer:: move_selection() {
    if (selection_x == -1) {
        selection_x=x;
        selection_y=y;
    }
}

void TextBuffer:: remove_selection() {
    selection_x=-1;
    selection_y=-1;
}

void TextBuffer:: copy_selection(vector< vector<string> >& clipboard) {
    if (selection_y == -1 || (selection_y == y && selection_x == x)) {
        return;
    }

    int begin_y, begin_x, end_y, end_x;
    if (selection_y > y || (selection_y == y && selection_x > x)) {
        begin_y=y;
        begin_x=x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
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

void TextBuffer:: cut_selection(vector< vector<string> >& clipboard) {
    if (selection_y == -1 || (selection_y == y && selection_x == x)) {
        return;
    }

    int begin_y, begin_x, end_y, end_x;
    if (selection_y > y || (selection_y == y && selection_x > x)) {
        begin_y=y;
        begin_x=x;
        end_y=selection_y;
        end_x=selection_x;
    }
    else {
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

    x=begin_x;
    y=begin_y;

    last_action=ACTION_CUT;
    last_action_modified=true;
}

void TextBuffer:: paste_selection(vector< vector<string> >& clipboard, int index) {
    if (clipboard.size() == 0) {
        return;
    }

    if (index < 0) {
        index=clipboard.size() - 1;
    }

    UndoInfo undo_info;
    undo_info.x=x;
    undo_info.y=y;
    undo_info.type=ACTION_PASTE;
    undo_info.index=index;
    undo_history.push_back(undo_info);

    insert_lines(clipboard[index]);

    last_action=ACTION_PASTE;
    last_action_modified=true;
}

void TextBuffer::undo(vector< vector<string> >& clipboard) {
    if (undo_history.size() <= undo_count) {
        return;
    }

    undo_count++;
    UndoInfo undo_info=undo_history[undo_history.size()-undo_count];

    switch(undo_info.type) {
        case ACTION_INSERT_CHAR:
            lines_[undo_info.y].erase(undo_info.x, undo_info.data.size());
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_DELETE_LINE:
        case ACTION_BACKSPACE:
        case ACTION_DELETE:
            lines_[undo_info.y].insert(undo_info.x, undo_info.data);
            x=undo_info.x+undo_info.data.size();
            y=undo_info.y;
            break;
        case ACTION_MOVE:
        case ACTION_CUT:
            x=undo_info.x;
            y=undo_info.y;
            paste_selection(clipboard, undo_info.index);
            break;
        case ACTION_COPY:
        case ACTION_PASTE: {
            vector<string> pasted=clipboard[undo_info.index];
            lines_[undo_info.y].erase(undo_info.x, pasted[0].size());
            lines_[undo_info.y] += lines_[undo_info.y + pasted.size() - 1].substr(pasted.back().size());
            lines_.erase(lines_.begin() + undo_info.y + 1, lines_.begin() + undo_info.y + pasted.size() - 1);
            x=undo_info.x;
            y=undo_info.y;
            }
            break;
        case ACTION_SPLIT_LINE:
            lines_[undo_info.y] += lines_[undo_info.y+1];
            lines_.erase(lines_.begin() + undo_info.y+1);
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_MERGE_LINE:
            lines_.insert(lines_.begin() + undo_info.y + 1, lines_[undo_info.y].substr(undo_info.x));
            lines_[undo_info.y].erase(undo_info.x);
            x=0;
            y=undo_info.y+1;
            break;
        case ACTION_REPLACE:
            lines_[undo_info.y].replace(undo_info.x,
                                        undo_info.index,
                                        undo_info.data.substr(undo_info.index));
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_UNDO:
        case ACTION_REDO:
        case ACTION_NONE: break;
    }

    last_action=ACTION_UNDO;
    last_action_modified=true;
}

void TextBuffer::redo(vector< vector<string> >& clipboard) {
    if (undo_count == 0) {
        return;
    }

    UndoInfo undo_info=undo_history[undo_history.size()-undo_count];
    undo_count--;

    switch(undo_info.type) {
        case ACTION_INSERT_CHAR:
            lines_[undo_info.y].insert(undo_info.x, undo_info.data);
            x=undo_info.x+undo_info.data.size();
            y=undo_info.y;
            break;
        case ACTION_BACKSPACE:
        case ACTION_DELETE:
            lines_[undo_info.y].erase(undo_info.x, undo_info.data.size());
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_DELETE_LINE:
            break;
        case ACTION_MOVE:
            break;
        case ACTION_CUT:
            break;
        case ACTION_COPY:
        case ACTION_PASTE:
            x=undo_info.x;
            y=undo_info.y;
            paste_selection(clipboard, undo_info.index);
            break;
        case ACTION_SPLIT_LINE:
            lines_.insert(lines_.begin() + undo_info.y + 1, lines_[undo_info.y].substr(undo_info.x));
            lines_[undo_info.y].erase(undo_info.x);
            x=0;
            y=undo_info.y+1;
            break;
        case ACTION_MERGE_LINE:
            lines_[undo_info.y] += lines_[undo_info.y+1];
            lines_.erase(lines_.begin() + undo_info.y+1);
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_REPLACE:
            lines_[undo_info.y].replace(undo_info.x, undo_info.data.length()-undo_info.index,
                                        undo_info.data.substr(0, undo_info.index));
            x=undo_info.x;
            y=undo_info.y;
            break;
        case ACTION_UNDO:
        case ACTION_REDO:
        case ACTION_NONE: break;
    }

    last_action=ACTION_REDO;
    last_action_modified=true;
}


bool TextBuffer::find_text(string find_what) {
    string line=lines_[y];
    size_t pos=line.find(find_what, x);
    if (pos != string::npos) {
        x=pos+find_what.length();
        selection_x=pos;
        selection_y=y;
        last_action=ACTION_MOVE;
        return true;
    }

    for (size_t row=y+1; row < lines_.size(); row++) {
        string line=lines_[row];
        size_t pos=line.find(find_what);
        if (pos != string::npos) {
            x=pos+find_what.length();
            y=row;
            selection_x=pos;
            selection_y=row;
            scroll=row;
            last_action=ACTION_MOVE;
            return true;
        }
    }

    return false;
}

bool TextBuffer::replace(string what, string with) {
    string &line=lines_[y];
    size_t pos=line.find(what, x);
    if (pos != string::npos) {
        UndoInfo undo_info;
        undo_info.x=pos;
        undo_info.y=y;
        undo_info.type=ACTION_REPLACE;
        undo_info.index=with.length();
        undo_info.data+=with;
        undo_info.data+=what;
        undo_history.push_back(undo_info);

        line.replace(pos, what.length(), with);
        x=pos+with.length();
        last_action=ACTION_REPLACE;
        last_action_modified=true;

        return true;
    }

    for (size_t row=y+1; row < lines_.size(); row++) {
        string &line=lines_[row];
        size_t pos=line.find(what);
        if (pos != string::npos) {
            UndoInfo undo_info;
            undo_info.x=pos;
            undo_info.y=row;
            undo_info.type=ACTION_REPLACE;
            undo_info.index=with.length();
            undo_info.data+=with;
            undo_info.data+=what;
            undo_history.push_back(undo_info);

            line.replace(pos, what.length(), with);
            x=pos+with.length();
            y=row;
            scroll=row;
            last_action=ACTION_REPLACE;
            last_action_modified=true;
            return true;
        }
    }

    return false;
}
