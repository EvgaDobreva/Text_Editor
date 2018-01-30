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

    //start_color();
    //init_pair(1, COLOR_GREEN, COLOR_BLACK);
    //attron(COLOR_PAIR(1));

    struct termios options;
    tcgetattr(2, &options);
    options.c_iflag &= ~(IXON|IXOFF);
    tcsetattr(2, TCSAFLUSH, &options);

    int file_buffer_x=0;
    int file_buffer_width=50;

    int debug_buffer_x=file_buffer_width+file_buffer_x;
    int debug_buffer_width=0;

    vector< vector<string> > clipboard;

    TextBuffer debug_buffer;
    debug_buffer.init_empty();

    TextBuffer file_buffer;
    file_buffer.load_file(filename);
    file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer);
    debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);


    while (1) {
        file_buffer.activate_buffer(file_buffer_x);
        int character = getch();
        if (character == 17) {
            break;
        }
        ostringstream status;
        switch(character) {
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
        case 4: // Ctrl+D=4
            file_buffer.delete_line(clipboard);
            status << "Line deleted";
            break;
        case KEY_BACKSPACE: // backspace
        case 127:
            file_buffer.key_backspace();
            status << "Deleting using Backspace";
            break;
        case KEY_DC: // delete
            file_buffer.key_delete();
            status << "Deleting using Delete";
            break;
        case '\n':
            file_buffer.key_enter();
            status << "New line using Enter";
            break;
        case 19: // Ctrl+S
            file_buffer.save(filename);
            status << "File Saved";
            break;
        case 2: // Ctrl+B
            file_buffer.word_backwards();
            status << "Moved backwards by word";
            break;
        case 1: // Ctrl+A
            file_buffer.line_begin();
            status << "Beginning of the line";
            break;
        case 5: // Ctrl+E
            file_buffer.line_end();
            status << "End of the line";
            break;
        case '\0':
            file_buffer.set_selection();
            status << "Set selection";
            break;
        case KEY_SLEFT:
            file_buffer.move_selection();
            file_buffer.key_left();
            status << "Moving selection using left";
            break;
        case KEY_SRIGHT:
            file_buffer.move_selection();
            file_buffer.key_right();
            status << "Moving selection using right";
            break;
        case 6: // Ctrl+F
            file_buffer.find_text();
            status << "Find text";
            break;
        case 23: // Ctrl+W
            file_buffer.cut_selection(clipboard);
            file_buffer.remove_selection();
            status << "Cut";
            break;
        case 22: // Ctrl+V
            file_buffer.paste_selection(clipboard);
            status << "Paste";
            break;
        case 25: // Ctrl+Y
            file_buffer.redo();
            status << "Redo";
            break;
        case 27: // ESC (alt was pressed along with another key)
            character=getch();
            switch(character) {
            case 'a': // alt+a
                file_buffer.file_begin();
                status << "Beginning of the file";
                break;
            case 'e': // alt+e
                file_buffer.file_end();
                status << "End of file";
                break;
            case 'f': // alt+f
                file_buffer.word_forward();
                status << "Moved forward by word";
                break;
            case 'w': // alt+w
                file_buffer.copy_selection(clipboard);
                file_buffer.remove_selection();
                status << "Copy";
                break;
            case 'z':
                file_buffer.undo(clipboard);
                status << "Undo";
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

        file_buffer.update(file_buffer_x, file_buffer_width, clipboard, &debug_buffer);
        debug_buffer.update(debug_buffer_x, debug_buffer_width, clipboard);
    }
    endwin();
    return 0;
}
/*
    TODO:
    Да се оправи обратното изрязване (отдясно наляво)
    Merge_line и Split_line да се изнесат във функция
    Да се оправи undo за cut
    Да се направи undo за paste
    
    1. оцветяване на ключови думи в с++
    2. търсене и замяна на текст
    ! Документ - описване на увода и първата част
*/
