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
    int scroll;
    int x;
    int y;
public:    
    FileContentBuffer(string filename);
    void load();
    void print();
    void save();
    void delete_line();
    void delete_char(int direction);
    void insert_char(char cursor);
    void new_line();
    int get_x();
    int get_y();
    void key_left();
    void key_right();
    void key_up();
    void key_down();
    void key_backspace();
    void key_delete();
    void key_enter();
    void word_forward();
    void word_backwards();
    void line_begin();
    void line_end();
    void file_begin();
    void file_end();
    void set_selection();
    void move_selection();
    void remove_selection();
    void copy_selection(vector< vector<string> >& clipboard);
    void cut_selection(vector< vector<string> >& clipboard);
    void paste_selection(vector< vector<string> >& clipboard);
    void find_text();
};

#endif
