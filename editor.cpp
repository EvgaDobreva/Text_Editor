#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <termios.h>
#include <string.h>
#include <map>

#include "textbuffer.hpp"

using namespace std;


enum KeyAction {
    KEYACTION_NONE,
    KEYACTION_LEFT,
    KEYACTION_RIGHT,
    KEYACTION_UP,
    KEYACTION_DOWN,
    KEYACTION_SAVE,
    KEYACTION_QUIT,
    KEYACTION_DELETE_LINE,
    KEYACTION_BACKSPACE,
    KEYACTION_DELETE,
    KEYACTION_NEW_LINE,
    KEYACTION_WORD_FORWARD,
    KEYACTION_WORD_BACKWARDS,
    KEYACTION_LINE_BEGIN,
    KEYACTION_LINE_END,
    KEYACTION_FILE_BEGIN,
    KEYACTION_FILE_END,
    KEYACTION_SELECT_LEFT,
    KEYACTION_SELECT_RIGHT,
    KEYACTION_CUT,
    KEYACTION_COPY,
    KEYACTION_PASTE,
    KEYACTION_UNDO,
    KEYACTION_REDO,
    KEYACTION_FIND,
    KEYACTION_REPLACE,
};


void set_default_controls(map<string, KeyAction> *controls) {
    (*controls)["left"] = KEYACTION_LEFT;
    (*controls)["right"] = KEYACTION_RIGHT;
    (*controls)["up"] = KEYACTION_UP;
    (*controls)["down"] = KEYACTION_DOWN;
    (*controls)["^s"] = KEYACTION_SAVE;
    (*controls)["^q"] = KEYACTION_QUIT;
    (*controls)["^d"] = KEYACTION_DELETE_LINE;
    (*controls)["backspace"] = KEYACTION_BACKSPACE;
    (*controls)["delete"] = KEYACTION_DELETE;
    (*controls)["enter"] = KEYACTION_NEW_LINE;
    (*controls)["^w"] = KEYACTION_WORD_FORWARD;
    (*controls)["^b"] = KEYACTION_WORD_BACKWARDS;
    (*controls)["^a"] = KEYACTION_LINE_BEGIN;
    (*controls)["^e"] = KEYACTION_LINE_END;
    (*controls)["Ma"] = KEYACTION_FILE_BEGIN;
    (*controls)["Me"] = KEYACTION_FILE_END;
    (*controls)["sleft"] = KEYACTION_SELECT_LEFT;
    (*controls)["sright"] = KEYACTION_SELECT_RIGHT;
    (*controls)["Mx"] = KEYACTION_CUT;
    (*controls)["Mw"] = KEYACTION_COPY;
    (*controls)["^v"] = KEYACTION_PASTE;
    (*controls)["Mz"] = KEYACTION_UNDO;
    (*controls)["^y"] = KEYACTION_REDO;
    (*controls)["^f"] = KEYACTION_FIND;
    (*controls)["^r"] = KEYACTION_REPLACE;
}


void load_controls(map<string, KeyAction> *controls, const char* filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        return;
    }

    string line;
    while (!file.eof()) {
        getline(file, line);
        if (line != "") {
            int split = line.find(' ');
            string action = line.substr(0, split);
            string key = line.substr(split + 1);

            if      (action == "left") (*controls)[key] = KEYACTION_LEFT;
            else if (action == "right") (*controls)[key] = KEYACTION_RIGHT;
            else if (action == "up") (*controls)[key] = KEYACTION_UP;
            else if (action == "down") (*controls)[key] = KEYACTION_DOWN;
            else if (action == "save") (*controls)[key] = KEYACTION_SAVE;
            else if (action == "quit") (*controls)[key] = KEYACTION_QUIT;
            else if (action == "delete_line") (*controls)[key] = KEYACTION_DELETE_LINE;
            else if (action == "backspace") (*controls)[key] = KEYACTION_BACKSPACE;
            else if (action == "delete") (*controls)[key] = KEYACTION_DELETE;
            else if (action == "new_line") (*controls)[key] = KEYACTION_NEW_LINE;
            else if (action == "word_forward") (*controls)[key] = KEYACTION_WORD_FORWARD;
            else if (action == "word_backwards") (*controls)[key] = KEYACTION_WORD_BACKWARDS;
            else if (action == "line_begin") (*controls)[key] = KEYACTION_LINE_BEGIN;
            else if (action == "line_end") (*controls)[key] = KEYACTION_LINE_END;
            else if (action == "file_begin") (*controls)[key] = KEYACTION_FILE_BEGIN;
            else if (action == "file_end") (*controls)[key] = KEYACTION_FILE_END;
            else if (action == "select_left") (*controls)[key] = KEYACTION_SELECT_LEFT;
            else if (action == "select_right") (*controls)[key] = KEYACTION_SELECT_RIGHT;
            else if (action == "cut") (*controls)[key] = KEYACTION_CUT;
            else if (action == "copy") (*controls)[key] = KEYACTION_COPY;
            else if (action == "paste") (*controls)[key] = KEYACTION_PASTE;
            else if (action == "undo") (*controls)[key] = KEYACTION_UNDO;
            else if (action == "redo") (*controls)[key] = KEYACTION_REDO;
            else if (action == "find") (*controls)[key] = KEYACTION_FIND;
            else if (action == "replace") (*controls)[key] = KEYACTION_REPLACE;
            else {
                cout << "Unknown action: " << action << endl;
            }
        }
    }
}


string input_line(const char *name) {
    move(LINES-1, 0);
    attron(A_REVERSE);
    for(int i=0; i < COLS;i++) {
      printw(" ");
    }
    move(LINES-1, 0);

    printw("%s: ", name);
    string input;
    bool should_stop_input=false;
    while (!should_stop_input) {
        int c = getch();
        switch (c) {
        case '\n':
            should_stop_input=true;
            break;
        default:
            input+=c;
            printw("%c", c);
        }
    }
    attroff(A_REVERSE);

    return input;
}


int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "Need more arguments!" << endl;
        return 1;
    }

    const char* filename=argv[1];

    map<string, KeyAction> controls;
    set_default_controls (&controls);
    load_controls (&controls, "editorrc");

    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    use_default_colors();
    start_color();
    init_pair(1, COLOR_GREEN, -1); // data_types
    init_pair(2, COLOR_RED, -1); // keywords
    init_pair(3, COLOR_BLUE, -1); // numbers
    init_pair(4, COLOR_YELLOW, -1); // strings
    init_pair(5, COLOR_MAGENTA, -1); // comments

    struct termios options;
    tcgetattr(2, &options);
    options.c_iflag &= ~(IXON|IXOFF);
    tcsetattr(2, TCSAFLUSH, &options);

    int file_buffer_x=0;
    int file_buffer_width=60;

    int debug_buffer_x=file_buffer_width+file_buffer_x;
    int debug_buffer_width=0;

    TextBuffer debug_buffer;
    debug_buffer.init_empty();

    string last_search;
    string last_replace_what;
    vector< vector<string> > clipboard;

    TextBuffer file_buffer;
    file_buffer.load_file(filename);
    file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer);
    debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);

    bool should_close_program = false;
    while (!should_close_program) {
        file_buffer.activate_buffer(file_buffer_x);
        int character = getch();
        ostringstream status;

        string key;
        switch (character) {
        case '\e': {
            key = 'M';
            key += getch();
            break;
        }
        case '\0':
            key = "^space";
            break;
        case '\n':
            key = "enter";
            break;
        case KEY_LEFT:
            key = "left";
            break;
        case KEY_RIGHT:
            key = "right";
            break;
        case KEY_UP:
            key = "up";
            break;
        case KEY_DOWN:
            key = "down";
            break;
        case KEY_SLEFT:
            key = "sleft";
            break;
        case KEY_SRIGHT:
            key = "sright";
            break;
        case 127:
            key = "backspace";
            break;
        case KEY_BACKSPACE:
            key = "^backspace";
            break;
        case KEY_DC:
            key = "delete";
            break;
        default:
            if (character >= 1 && character <= 26) {
                key = '^';
                key += ('a' - 1) + character;
            }
            else if (character >= ' ' && character <= '~') {
                file_buffer.insert_char(char(character));
            }
        }

        KeyAction keyaction = controls[key.c_str()];

        switch (keyaction) {
        case KEYACTION_NONE:
            break;
        case KEYACTION_LEFT:
            file_buffer.key_left();
            file_buffer.remove_selection();
            status << "Left";
            break;
        case KEYACTION_RIGHT:
            file_buffer.key_right();
            file_buffer.remove_selection();
            status << "Right";
            break;
        case KEYACTION_UP:
            file_buffer.key_up();
            status << "Up";
            break;
        case KEYACTION_DOWN:
            file_buffer.key_down();
            status << "Down";
            break;
        case KEYACTION_SAVE:
            file_buffer.save(filename);
            status << "File Saved";
            break;
        case KEYACTION_QUIT:
            should_close_program=true;
            status << "Quit";
            break;
        case KEYACTION_DELETE_LINE:
            file_buffer.delete_line(clipboard);
            status << "Line Deleted";
            break;
        case KEYACTION_BACKSPACE:
            file_buffer.key_backspace();
            status << "Deleting (Backspace)";
            break;
        case KEYACTION_DELETE:
            file_buffer.key_delete();
            status << "Deleting (Delete)";
            break;
        case KEYACTION_NEW_LINE:
            file_buffer.key_enter();
            status << "New line (Enter)";
            break;
        case KEYACTION_WORD_FORWARD:
            file_buffer.word_forward();
            status << "Move forward by word";
            break;
        case KEYACTION_WORD_BACKWARDS:
            file_buffer.word_backwards();
            status << "Move backwards by word";
            break;
        case KEYACTION_LINE_BEGIN:
            file_buffer.line_begin();
            status << "Beginning of the line";
            break;
        case KEYACTION_LINE_END:
            file_buffer.line_end();
            status << "End of the line";
            break;
        case KEYACTION_FILE_BEGIN:
            file_buffer.file_begin();
            status << "Beginning of the file";
            break;
        case KEYACTION_FILE_END:
            file_buffer.file_end();
            status << "End of file";
            break;
        case KEYACTION_SELECT_LEFT:
            file_buffer.move_selection();
            file_buffer.key_left();
            status << "Selecting (Left)";
            break;
        case KEYACTION_SELECT_RIGHT:
            file_buffer.move_selection();
            file_buffer.key_right();
            status << "Selecting (Right)";
            break;
        case KEYACTION_CUT:
            file_buffer.cut_selection(clipboard);
            file_buffer.remove_selection();
            status << "Cut";
            break;
        case KEYACTION_COPY:
            file_buffer.copy_selection(clipboard);
            file_buffer.remove_selection();
            status << "Copy";
            break;
        case KEYACTION_PASTE:
            file_buffer.paste_selection(clipboard);
            status << "Paste";
            break;
        case KEYACTION_UNDO:
            file_buffer.undo(clipboard);
            status << "Undo";
            break;
        case KEYACTION_REDO:
            file_buffer.redo(clipboard);
            status << "Redo";
            break;
        case KEYACTION_FIND: {
                string find_what=input_line("Find");
                if (find_what != "") {
                    last_search=find_what;
                }
                else if (last_search != "") {
                    find_what=last_search;
                }

                if (find_what != "") {
                    bool found=file_buffer.find_text(find_what);
                    if (found) {
                        status << "Found \"" << find_what << '"';
                    }
                    else {
                        status << "Can't find \"" << find_what << '"';
                    }
                }
            }
            break;
        case KEYACTION_REPLACE: {
                string replace_what=input_line("Replace what");
                if (replace_what != "") {
                    last_replace_what = replace_what;
                }
                else if (last_replace_what != "") {
                    replace_what = last_replace_what;
                }

                if (replace_what != "") {
                    string replace_with=input_line("Replace with");

                    if (replace_what != "") {
                        size_t replaced_count=0;
                        while (file_buffer.replace(replace_what, replace_with)) {
                            replaced_count++;
                        }
                        status << "Replaced "
                               << replaced_count
                               << " occurrences of \""
                               << replace_what
                               << "\" with \""
                               << replace_with
                               << "\"";
                    }
                }
            }
            break;
        }

        clear();
        move(LINES-1, 0);
        attron(A_REVERSE);

        printw("Ln:%d, Col:%d, Copied:%d  %s ", file_buffer.get_y()+1, file_buffer.get_x()+1, clipboard.size(), status.str().c_str());
        for(int i=0; i < COLS;i++) {
            printw(" ");
        }
        move(LINES-1, COLS - strlen(filename) - 1);
        printw("%s ", filename);
        attroff(A_REVERSE);

        file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer);
        debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);
    }
    endwin();
    return 0;
}
