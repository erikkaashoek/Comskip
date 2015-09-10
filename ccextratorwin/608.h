#ifndef __608_H__
struct s_write;

void process608 (const unsigned char *data, int length, struct s_write *wb);
void get_char_in_latin_1 (unsigned char *buffer, unsigned char c);
void get_char_in_unicode (unsigned char *buffer, unsigned char c);
int get_char_in_utf_8 (unsigned char *buffer, unsigned char c);
unsigned char cctolower (unsigned char c);
unsigned char cctoupper (unsigned char c);

enum cc_modes
{
    MODE_POPUP = 0,
    MODE_ROLLUP_2 = 1,
    MODE_ROLLUP_3 = 2,
    MODE_ROLLUP_4 = 3,
	MODE_TEXT = 4
};

enum color_code
{
    COL_WHITE = 0,
    COL_GREEN = 1,
    COL_BLUE = 2,
    COL_CYAN = 3,
    COL_RED = 4,
    COL_YELLOW = 5,
    COL_MAGENTA = 6,
	COL_USERDEFINED = 7
};


enum font_bits
{
    FONT_REGULAR = 0,
    FONT_ITALICS = 1,
    FONT_UNDERLINED = 2,
    FONT_UNDERLINED_ITALICS = 3
};


typedef struct eia608_screen // A CC buffer
{
    unsigned char characters[15][33]; 
    unsigned char colors[15][33];
    unsigned char fonts[15][33]; // Extra char at the end for a 0
    int row_used[15]; // Any data in row?
    int empty; // Buffer completely empty?    	
} eia608_screen;

struct eia608
{
    struct eia608_screen buffer1;
    eia608_screen buffer2;  
    int cursor_row, cursor_column;
    int visible_buffer;
    int srt_counter; // Number of subs currently written
	int screenfuls_counter; // Number of meaningful screenfuls written
    unsigned current_visible_start_cc; // At what time did the current visible buffer became so?
    int  mode; //cc_modes
    unsigned char last_c1, last_c2;
    int channel; // Currently selected channel
    unsigned char color; // Color we are currently using to write
    unsigned char font; // Font we are currently using to write
    int rollup_base_row;
};



enum command_code
{
    COM_UNKNOWN = 0,
    COM_ERASEDISPLAYEDMEMORY = 1,
    COM_RESUMECAPTIONLOADING = 2,
    COM_ENDOFCAPTION = 3,
    COM_TABOFFSET1 = 4,
    COM_TABOFFSET2 = 5,
    COM_TABOFFSET3 = 6,
    COM_ROLLUP2 = 7,
    COM_ROLLUP3 = 8,
    COM_ROLLUP4 = 9,
    COM_CARRIAGERETURN = 10,
    COM_ERASENONDISPLAYEDMEMORY = 11,
    COM_BACKSPACE = 12,
	COM_RESUMETEXTDISPLAY = 13
};


extern const unsigned char pac2_attribs[32][3];

#define __608_H__
#endif
