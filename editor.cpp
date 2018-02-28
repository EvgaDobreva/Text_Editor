#include <iostream>
#include <ncurses.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <termios.h>
#include <string.h>

#include "textbuffer.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "Need more arguments!" << endl;
        return 1;
    }

    const char* filename=argv[1];

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

    vector< vector<string> > clipboard;
    bool find_text=false;

    TextBuffer file_buffer;
    file_buffer.load_file(filename);
    file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer, find_text);
    debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);

    while (1) {
        file_buffer.activate_buffer(file_buffer_x);
        int character = getch();
        if (character == 17) {
            break;
        }
        ostringstream status;
        switch(character) {
        case 19: // ctrl+s
            file_buffer.save(filename);
            status << "File Saved";
            break;
        case KEY_LEFT: // left
            file_buffer.key_left();
            file_buffer.remove_selection();
            status << "Left";
            break;
        case KEY_RIGHT: // right
            file_buffer.key_right();
            file_buffer.remove_selection();
            status << "Right";
            break;
        case KEY_UP: // up
            file_buffer.key_up();
            status << "Up";
            break;
        case KEY_DOWN: // down
            file_buffer.key_down();
            status << "Down";
            break;
        case '\0':
            file_buffer.set_selection();
            status << "Set Selection";
            break;
        case KEY_SLEFT: // shift+left
            file_buffer.move_selection();
            file_buffer.key_left();
            status << "Selecting (Left)";
            break;
        case KEY_SRIGHT: // shift+right
            file_buffer.move_selection();
            file_buffer.key_right();
            status << "Selecting (Right)";
            break;
        case 23: // ctrl+w
            file_buffer.word_forward();
            status << "Move forward by word";
            break;
        case 2: // ctrl+b
            file_buffer.line_begin();
            status << "Beginning of the line";
            break;
        case 5: // ctrl+e
            file_buffer.line_end();
            status << "End of the line";
            break;
        case KEY_BACKSPACE: // backspace
        case 127:
            file_buffer.key_backspace();
            status << "Deleting (Backspace)";
            break;
        case KEY_DC: // delete
            file_buffer.key_delete();
            status << "Deleting (Delete)";
            break;
        case 4: // ctrl+d
            file_buffer.delete_line(clipboard);
            status << "Line Deleted";
            break;
        case '\n': // enter
            file_buffer.key_enter();
            status << "New line (Enter)";
            break;
        case 6: // ctrl+f
            file_buffer.find_text();
            status << "Find text";
            break;
        case 27: // ESC (alt was pressed along with another key)
            character=getch();
            switch(character) {
            case 'w':
                file_buffer.word_backwards();
                status << "Move backwards by word";
                break;
            case 'b': // alt+b
                file_buffer.file_begin();
                status << "Beginning of the file";
                break;
            case 'e': // alt+e
                file_buffer.file_end();
                status << "End of file";
                break;
            case 'c': // alt+c
                file_buffer.copy_selection(clipboard);
                file_buffer.remove_selection();
                status << "Copy";
                break;
            case 'x': // alt+x
                file_buffer.cut_selection(clipboard);
                file_buffer.remove_selection();
                status << "Cut";
                break;
            case 'v': // alt+v
                file_buffer.paste_selection(clipboard);
                status << "Paste";
                break;
            case 'z': // alt+z
                file_buffer.undo(clipboard);
                status << "Undo";
                break;
            case 'y': // alt+y
                file_buffer.redo(clipboard);
                status << "Redo";
                break;
            default:
                status << "Unknown command: '" << (char) character << '\'';
            }
            break;
        default:
            if (character >= ' ' && character <= '~') {
                file_buffer.insert_char(char(character));
            }
            else {
                status << "Unknown command: '";
                if (character == 0) {
                   status << "^@";
                }
                else {
                    status << (char) character;
                }
                status << '\'';
            }
        }

        clear();
        move(LINES-1, 0);
        attron(A_REVERSE);
        
        printw("Ln:%d, Col:%d Copied:%d  %s ", file_buffer.get_y()+1, file_buffer.get_x()+1, clipboard.size(), status.str().c_str());
        for(int i=0; i < COLS;i++) {
            printw(" ");
        }
        move(LINES-1, COLS - strlen(filename) - 1);
        printw("%s ", filename);
        attroff(A_REVERSE);

        file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer, find_text);
        debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);
    }
    endwin();
    return 0;
}
