#ifndef textbuffer_h
#define textbuffer_h

#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>

using namespace std;

enum ActionType {
    ACTION_NONE,
    ACTION_DELETE_LINE,
    ACTION_BACKSPACE,
    ACTION_DELETE,
    ACTION_MOVE,
    ACTION_CUT,
    ACTION_COPY,
    ACTION_PASTE,
    ACTION_INSERT_CHAR,
    ACTION_SPLIT_LINE,
    ACTION_MERGE_LINE,
    ACTION_UNDO,
    ACTION_REDO,
    ACTION_REPLACE,
};

struct UndoInfo {
    int x;
    int y;
    ActionType type;
    int index;
    string data;
};

class TextBuffer {
    vector<string> lines_;
    int selection_x;
    int selection_y;
    int scroll;
    int x;
    int y;
    vector<UndoInfo> undo_history;
    ActionType last_action;
    size_t undo_count;
    bool last_action_modified;
public:
    TextBuffer();
    void load_file(string filename);
    void save(string filename);
    void init_empty();
    void clear();
    void insert_lines(vector<string> new_lines);
    void update(int buffer_x, int width, vector< vector<string> > clipboard, TextBuffer* debug_buffer=NULL);
    void activate_buffer(int buffer_x);
    void insert_line(string line);
    void delete_line(vector< vector<string> >& clipboard);
    void insert_char(char cursor);
    void key_backspace();
    void key_delete();
    void key_enter();
    int get_x();
    int get_y();
    void key_left();
    void key_right();
    void key_up();
    void key_down();
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
    void paste_selection(vector< vector<string> >& clipboard, int index=-1);
    void undo(vector< vector<string> >& clipboard);
    void redo(vector< vector<string> >& clipboard);
    bool find_text(string find_What);
    bool replace(string what, string with);
};

#endif
