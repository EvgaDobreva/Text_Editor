#ifndef filecontentbuffer_h
#define fielcontentbuffer_h

#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <string.h>

using namespace std;

class FileContentBuffer {
    string filename_;
    vector<string> lines_;
    int selection_x;
    int selection_y;
public:    
    FileContentBuffer(string filename);
    void load();
    void print(int cursor_y, int cursor_x);
    void save();
    void delete_line(int y);
    void delete_char(int x, int y, int direction);
    void insert_char(int& x, int& y, char cursor);
    void new_line(int y);
    void key_left(int& y, int& x);
    void key_right(int& y, int& x);
    void key_up(int& y, int& x);
    void key_down(int& y, int& x);
    void key_backspace(int& y, int& x);
    void key_delete(int& y, int& x);
    void key_enter(int& y, int& x);
    void word_forward(int& y, int& x);
    void word_backwards(int& y, int& x);
    void line_begin(int& x);
    void line_end(int& y, int& x);
    void file_begin(int& y, int& x);
    void file_end(int& y, int& x);
    void set_selection(int y, int x);
    void move_selection(int y, int x);
    void remove_selection();
    void copy_selection(int y, int x, vector< vector<string> >& clipboard);
    void cut_selection(int& y, int& x, vector< vector<string> >& clipboard);
    void paste_selection(int& y, int& x, vector< vector<string> >& clipboard);
    void find_text();
};

#endif
