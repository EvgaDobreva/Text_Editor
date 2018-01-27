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

/*enum UndoType {
    UNDO_CLIPBOARD,
    UNDO_SMALL_CLIPBOARD,
    UNDO_CONCAT_LINES,
    UNDO_SPLIT_LINES,
    UNDO_INSERT_WORD,
};*/

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
    ACTION_NEW_LINE,
    ACTION_UNDO,
};

struct UndoInfo {
    int x;
    int y;
    ActionType type;
    int index;
};

class FileContentBuffer {
    string filename_;
    vector<string> lines_;
    int selection_x;
    int selection_y;
    int scroll;
    int x;
    int y;
    vector<UndoInfo> undo_history;
    ActionType last_action;
    vector<string> small_clipboard;
    size_t undo_count;
public:    
    FileContentBuffer(string filename);
    void load();
    void print();
    void save();
    void delete_line(vector< vector<string> >& clipboard);
    void insert_char(char cursor);
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
    void undo(vector< vector<string> >& clipboard);
    void redo();
    void find_text();
};

#endif
