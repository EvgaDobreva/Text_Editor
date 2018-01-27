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

#include "filecontentbuffer.hpp"

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

    FileContentBuffer file(filename);
    file.load();
    file.print();

    int cursor;
    move(0, 0);
    refresh();

    vector< vector<string> > clipboard;

    while ((cursor = getch()) != 17) {
        ostringstream status;
        switch(cursor) {        
        case KEY_LEFT: // left
            file.key_left();
            file.remove_selection();
            status << "Left";
            break;
        case KEY_RIGHT: // right
            file.key_right();
            file.remove_selection();
            status << "Right";
            break;
        case KEY_UP: // up
            file.key_up();
            status << "Up";
            break;
        case KEY_DOWN: // down
            file.key_down();
            status << "Down";
            break;
        case 4: // Ctrl+D=4
            file.delete_line(clipboard);
            status << "Line deleted";
            break;
        case KEY_BACKSPACE: // backspace
        case 127:
            file.key_backspace();
            status << "Deleting using Backspace";
            break;
        case KEY_DC: // delete
            file.key_delete();
            status << "Deleting using Delete";
            break;
        case '\n':
            file.key_enter();
            status << "New line using Enter";
            break;
        case 19: // Ctrl+S
            file.save();
            status << "File Saved";
            break;
        case 2: // Ctrl+B
            file.word_backwards();
            status << "Moved backwards by word";
            break;
        case 1: // Ctrl+A
            file.line_begin();
            status << "Beginning of the line";
            break;
        case 5: // Ctrl+E
            file.line_end();
            status << "End of the line";
            break;
        case '\0':
            file.set_selection();
            status << "Set selection";
            break;
        case KEY_SLEFT:
            file.move_selection();
            file.key_left();
            status << "Moving selection using left";
            break;
        case KEY_SRIGHT:
            file.move_selection();
            file.key_right();
            status << "Moving selection using right";
            break;
        case 6: // Ctrl+F
            file.find_text();
            status << "Find text";
            break;
        case 23: // Ctrl+W
            file.cut_selection(clipboard);
            file.remove_selection();
            status << "Cut";
            break;
        case 22: // Ctrl+V
            file.paste_selection(clipboard);
            status << "Paste";
        case 25: // Ctrl+Y
            file.redo();
            status << "Redo";
            break;
        case 27: // ESC (alt was pressed along with another key)
            cursor=getch();
            switch(cursor) {
            case 'a': // alt+a
                file.file_begin();
                status << "Beginning of the file";
                break;
            case 'e': // alt+e
                file.file_end();
                status << "End of file";
                break;
            case 'f': // alt+f
                file.word_forward();
                status << "Moved forward by word";
                break;
            case 'w': // alt+w
                file.copy_selection(clipboard);
                file.remove_selection();
                status << "Copy";
                break;
            case 'z':
                file.undo(clipboard);
                status << "Undo";
                break;
            default:
                status << "Unknown command: '" << (char) cursor << '\'';
            }
            break;
        default:
            if (cursor >= ' ' && cursor <= '~') {
                file.insert_char(char(cursor));
            }
            else {
                status << "Unknown command: '";
                if (cursor == 0) {
                   status << "^@";
                }
                else {
                    status << (char) cursor;
                }
                status << '\'';
            }
        }
        
        clear();
        move(LINES-1, 0);
        attron(A_REVERSE);
        printw("Ln:%d, Col:%d Copied:%d  %s ", file.get_y()+1, file.get_x()+1, clipboard.size(), status.str().c_str());
        for(int i=0; i < COLS;i++) {
            printw(" ");
        }
        move(LINES-1, COLS - strlen(filename) - 1);
        printw("%s ", filename);
        attroff(A_REVERSE);
        
        file.print();
        refresh();
    }
    endwin();
    return 0;
}
/*
    TODO:
    1. оцветяване на ключови думи в с++
    2. търсене и замяна на текст    
    ! Документ - описване на увода и първата част
*/
