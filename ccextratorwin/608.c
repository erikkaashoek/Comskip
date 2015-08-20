#include <stdio.h>
#include "608.h"
#include "ccextractor.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int     rowdata[] = {11,-1,1,2,3,4,12,13,14,15,5,6,7,8,9,10};
// Relationship between the first PAC byte and the row number

const unsigned char pac2_attribs[][3]= // Color, font, ident
{
    {COL_WHITE,     FONT_REGULAR,               0},  // 0x40 || 0x60 
    {COL_WHITE,     FONT_UNDERLINED,            0},  // 0x41 || 0x61
    {COL_GREEN,     FONT_REGULAR,               0},  // 0x42 || 0x62
    {COL_GREEN,     FONT_UNDERLINED,            0},  // 0x43 || 0x63
    {COL_BLUE,      FONT_REGULAR,               0},  // 0x44 || 0x64
    {COL_BLUE,      FONT_UNDERLINED,            0},  // 0x45 || 0x65
    {COL_CYAN,      FONT_REGULAR,               0},  // 0x46 || 0x66
    {COL_CYAN,      FONT_UNDERLINED,            0},  // 0x47 || 0x67
    {COL_RED,       FONT_REGULAR,               0},  // 0x48 || 0x68
    {COL_RED,       FONT_UNDERLINED,            0},  // 0x49 || 0x69
    {COL_YELLOW,    FONT_REGULAR,               0},  // 0x4a || 0x6a
    {COL_YELLOW,    FONT_UNDERLINED,            0},  // 0x4b || 0x6b
    {COL_MAGENTA,   FONT_REGULAR,               0},  // 0x4c || 0x6c
    {COL_MAGENTA,   FONT_UNDERLINED,            0},  // 0x4d || 0x6d
    {COL_WHITE,     FONT_ITALICS,               0},  // 0x4e || 0x6e
    {COL_WHITE,     FONT_UNDERLINED_ITALICS,    0},  // 0x4f || 0x6f
    {COL_WHITE,     FONT_REGULAR,               0},  // 0x50 || 0x70
    {COL_WHITE,     FONT_UNDERLINED,            0},  // 0x51 || 0x71
    {COL_WHITE,     FONT_REGULAR,               4},  // 0x52 || 0x72
    {COL_WHITE,     FONT_UNDERLINED,            4},  // 0x53 || 0x73
    {COL_WHITE,     FONT_REGULAR,               8},  // 0x54 || 0x74
    {COL_WHITE,     FONT_UNDERLINED,            8},  // 0x55 || 0x75
    {COL_WHITE,     FONT_REGULAR,               12}, // 0x56 || 0x76
    {COL_WHITE,     FONT_UNDERLINED,            12}, // 0x57 || 0x77
    {COL_WHITE,     FONT_REGULAR,               16}, // 0x58 || 0x78
    {COL_WHITE,     FONT_UNDERLINED,            16}, // 0x59 || 0x79
    {COL_WHITE,     FONT_REGULAR,               20}, // 0x5a || 0x7a
    {COL_WHITE,     FONT_UNDERLINED,            20}, // 0x5b || 0x7b
    {COL_WHITE,     FONT_REGULAR,               24}, // 0x5c || 0x7c
    {COL_WHITE,     FONT_UNDERLINED,            24}, // 0x5d || 0x7d
    {COL_WHITE,     FONT_REGULAR,               28}, // 0x5e || 0x7e
    {COL_WHITE,     FONT_UNDERLINED,            28}  // 0x5f || 0x7f
};


#define true 1
#define false 0

int in_xds_mode = 0;
extern int ts_headers_total;

unsigned char enc_buffer[2048]; // Generic general purpose buffer
unsigned char str[2048]; // Another generic general purpose buffer
unsigned enc_buffer_used;

// Preencoded strings
unsigned char encoded_crlf[16]; 
unsigned int encoded_crlf_length;
unsigned char encoded_br[16];
unsigned int encoded_br_length;

unsigned char *subline; // Temp storage for .srt lines
int new_sentence=1; // Capitalize next letter?

// Default color
unsigned char usercolor_rgb[8]="";
int default_color=COL_WHITE; //color_code

const char *command_type[] =
{
    "Unknown",
    "EDM - EraseDisplayedMemory",
    "RCL - ResumeCaptionLoading",
    "EOC - End Of Caption",
    "TO1 - Tab Offset, 1 column",
    "TO2 - Tab Offset, 2 column",
    "TO3 - Tab Offset, 3 column",
    "RU2 - Roll up 2 rows",
    "RU3 - Roll up 3 rows",
    "RU4 - Roll up 4 rows",
    "CR  - Carriage Return",
    "ENM - Erase non-displayed memory",
    "BS  - Backspace",
	"RTD - Resume Text Display"
};

const char *font_text[]=
{
    "regular",
    "italics",
    "underlined",
    "underlined italics"
};

const char *cc_modes_text[]=
{
    "Pop-Up captions"
};

const char *color_text[][2]=
{
    {"white",""},
    {"green","<font color=\"#00ff00\">"},
    {"blue","<font color=\"#0000ff\">"},
    {"cyan","<font color=\"#00ffff\">"},
    {"red","<font color=\"#ff0000\">"},
    {"yellow","<font color=\"#ffff00\">"},
    {"magenta","<font color=\"#ff00ff\">"},
	{"userdefined","<font color=\""}
};

// Encodes a generic string. Note that since we use the encoders for closed caption
// data, text would have to be encoded as CCs... so using special characters here
// it's a bad idea. 
unsigned encode_line (unsigned char *buffer, unsigned char *text)
{ 
    unsigned bytes=0;
    while (*text)
    {		
        switch (encoding)
        {
            case ENC_UTF_8:
            case ENC_LATIN_1:
                *buffer=*text;
                bytes++;
                buffer++;
                break;
            case ENC_UNICODE:				
                *buffer=*text;				
                *(buffer+1)=0;
                bytes+=2;				
                buffer+=2;
                break;
        }		
        text++;
    }
    return bytes;
}

#define ISSEPARATOR(c) (c==' ' || c==0x89 || ispunct(c))

void correct_case (int line_num, struct eia608_screen *data)
{
	int i=0;
	while (i<spell_words)
	{
		char *c=(char *) data->characters[line_num];
		size_t len=strlen (spell_correct[i]);
		while ((c=strstr (c,spell_lower[i]))!=NULL)
		{
			// Make sure it's a whole word (start of line or
			// preceded by space, and end of line or followed by
			// space)
			unsigned char prev= (c==(char *) data->characters[line_num])?' ':*(c-1);
			unsigned char next=*(c+len);			
			if ( ISSEPARATOR(prev) && ISSEPARATOR(next))				
			{
				memcpy (c,spell_correct[i],len);
			}
			c++;
		}
		i++;
	}
}

void capitalize (int line_num, struct eia608_screen *data)
{	int i;
	for (i=0;i<32;i++)
	{
		switch (data->characters[line_num][i])
		{
			case ' ': 
			case 0x89: // This is a transparent space
				break; 
			case '.': // Fallthrough
			case '?': // Fallthrough
			case '!':
				new_sentence=1;
				break;
			default:
				if (new_sentence)			
					data->characters[line_num][i]=cctoupper (data->characters[line_num][i]);
				else
					data->characters[line_num][i]=cctolower (data->characters[line_num][i]);
				new_sentence=0;
				break;
		}
	}
}

unsigned get_decoder_line_encoded (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	int is_underlined;
    int col = COL_WHITE;
    int underlined = 0;
    int italics = 0;	
	int i;
    int bytes=0;
 	int has_ita;
    unsigned char *line = data->characters[line_num];	
    unsigned char *orig=buffer; // Keep for debugging
    for (i=0;i<32;i++)
    {	
        // Handle color
        int its_col = data->colors[line_num][i];
        if (its_col != col  && !nofontcolor)
        {
            if (col!=COL_WHITE) // We need to close the previous font tag
            {
                buffer+= encode_line (buffer,(unsigned char *) "</font>");
            }
            // Add new font tag
			buffer+=encode_line (buffer, (unsigned char*) color_text[its_col][1]);
			if (its_col==COL_USERDEFINED)
			{
				// The previous sentence doesn't copy the whole 
				// <font> tag, just up to the quote before the color
				buffer+=encode_line (buffer, (unsigned char*) usercolor_rgb);
				buffer+=encode_line (buffer, (unsigned char*) "\">");
			}			
				
            col = its_col;
        }
        // Handle underlined
        is_underlined = data->fonts[line_num][i] & FONT_UNDERLINED;
        if (is_underlined && underlined==0) // Open underline
        {
            buffer+=encode_line (buffer, (unsigned char *) "<u>");
        }
        if (is_underlined==0 && underlined) // Close underline
        {
            buffer+=encode_line (buffer, (unsigned char *) "</u>");
        } 
        underlined=is_underlined;
        // Handle italics
        has_ita = data->fonts[line_num][i] & FONT_ITALICS;		
        if (has_ita && italics==0) // Open italics
        {
            buffer+=encode_line (buffer, (unsigned char *) "<i>");
        }
        if (has_ita==0 && italics) // Close italics
        {
            buffer+=encode_line (buffer, (unsigned char *) "</i>");
        } 
        italics=has_ita;
        bytes=0;
        switch (encoding)
        {
            case ENC_UTF_8:
                bytes=get_char_in_utf_8 (buffer,line[i]);
                break;
            case ENC_LATIN_1:
                get_char_in_latin_1 (buffer,line[i]);
                bytes=1;
                break;
            case ENC_UNICODE:
                get_char_in_unicode (buffer,line[i]);
                bytes=2;				
                break;
        }
        buffer+=bytes;
    }
    if (italics)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</i>");
    }
    if (underlined)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</u>");
    }
    if (col != COL_WHITE && !nofontcolor)
    {
        buffer+=encode_line (buffer, (unsigned char *) "</font>");
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length
}


void clear_eia608_cc_buffer (struct eia608_screen *data)
{
	int i;
    for (i=0;i<15;i++)
    {
        memset(data->characters[i],' ',32);
        data->characters[i][32]=0;		
		memset (data->colors[i],default_color,33); 
        memset (data->fonts[i],FONT_REGULAR,33); 
        data->row_used[i]=0;
        data->empty=1;
    }
}

void init_eia608 (struct eia608 *data)
{
    data->cursor_column=0;
    data->cursor_row=0;
    clear_eia608_cc_buffer (&data->buffer1);
    clear_eia608_cc_buffer (&data->buffer2);
    data->visible_buffer=1;
    data->last_c1=0;
    data->last_c2=0;
    data->mode=MODE_POPUP;
    data->current_visible_start_cc=0;
    data->srt_counter=0;
	data->screenfuls_counter=0;
    data->channel=1;	
	data->color=default_color;
    data->font=FONT_REGULAR;
    data->rollup_base_row=14;
}

eia608_screen *get_writing_buffer (struct s_write *wb)
{
    eia608_screen *use_buffer=NULL;
    switch (wb->data608->mode)
    {
        case MODE_POPUP: // Write on the non-visible buffer
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer2;
            else
                use_buffer = &wb->data608->buffer1;
            break;
        case MODE_ROLLUP_2: // Write directly to screen
        case MODE_ROLLUP_3:
        case MODE_ROLLUP_4:
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer1;
            else
                use_buffer = &wb->data608->buffer2;
            break;
        default:
            printf ("Caption mode has an illegal value at get_writing_buffer(), this is a bug.\n");
            exit(500);
    }
    return use_buffer;
}

void write_char (const unsigned char c, struct s_write *wb)
{
	if (wb->data608->mode!=MODE_TEXT)
	{
		eia608_screen * use_buffer=get_writing_buffer(wb);
		/* printf ("\rWriting char [%c] at %s:%d:%d\n",c,
		use_buffer == &wb->data608->buffer1?"B1":"B2",
		wb->data608->cursor_row,wb->data608->cursor_column); */
		use_buffer->characters[wb->data608->cursor_row][wb->data608->cursor_column]=c;
		use_buffer->colors[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->color;
		use_buffer->fonts[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->font;	
		use_buffer->row_used[wb->data608->cursor_row]=1;
		use_buffer->empty=0;
		if (wb->data608->cursor_column<31)
			wb->data608->cursor_column++;
	}

}


void handle_text_attr (const unsigned char c1, const unsigned char c2, struct s_write *wb)
{
    if (debug_608)
        printf ("\r608: text_attr: %02X %02X\n",c1,c2);
    if ( ((c1!=0x11 && c1!=0x19) ||
        (c2<0x20 || c2>0x2f)) && debug_608)
    {
        printf ("\rThis is not a text attribute!\n");
    }
    else
    {
        int i = c2-0x20;
        wb->data608->color=pac2_attribs[i][0];
        wb->data608->font=pac2_attribs[i][1];
        if (debug_608)
            printf ("\rColor: %s,  font: %s\n",
            color_text[wb->data608->color][0],
            font_text[wb->data608->font]);
        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
    }
}

void mstotime (LONG milli, unsigned *hours, unsigned *minutes,
                     unsigned *seconds, unsigned *ms)
{
    // LONG milli = (LONG) ((ccblock*1000)/29.97);
    *ms=(unsigned) (milli%1000); // milliseconds
    milli=(milli-*ms)/1000;  // Remainder, in seconds
    *seconds = (int) (milli%60);
    milli=(milli-*seconds)/60; // Remainder, in minutes
    *minutes = (int) (milli%60);
    milli=(milli-*minutes)/60; // Remainder, in hours
    *hours=(int) milli;
}

void write_subtitle_file_footer (struct s_write *wb)
{
    switch (write_format)
    {
        case OF_SAMI:
            sprintf ((char *) str,"<BODY><SAMI>\n");
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                printf ("\r%s\n", str);
            }
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
            fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
            break;
    }
}

void fprintf_encoded (FILE *fh, char *string)
{
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) string);
    fwrite (enc_buffer,enc_buffer_used,1,fh);
}

void write_subtitle_file_header (struct s_write *wb)
{
    switch (write_format)
    {
        case OF_SRT: // Subrip subtitles have no header
            break; 
        case OF_SAMI: // This header brought to you by McPoodle's CCASDI  
            fprintf_encoded (wb->fh, "<SAMI>\n");
            fprintf_encoded (wb->fh, "<HEAD>\n");
            fprintf_encoded (wb->fh, "<STYLE TYPE=\"text/css\">\n");
            fprintf_encoded (wb->fh, "<--\n");
            fprintf_encoded (wb->fh, "P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n");
            fprintf_encoded (wb->fh, "   text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n");
            fprintf_encoded (wb->fh, ".UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n");
            fprintf_encoded (wb->fh, "-->\n");
            fprintf_encoded (wb->fh, "</STYLE>\n");
            fprintf_encoded (wb->fh, "</HEAD>\n\n");
            fprintf_encoded (wb->fh, "<BODY>\n");
            break;
    }
}

int write_cc_buffer_as_srt (struct eia608_screen *data, struct s_write *wb)
{
	int i;
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;
	int wrote_something = 0;
    // This is the only place we take the number of blocks written in other files,
    // we don't want the times to reset in the middle of our file.
    unsigned pg = c1global? c1global:c2global; // Any non-zero global will do
    char timeline[128];   
	long ms_end;
	LONG ms_start= (LONG) (((wb->data608->current_visible_start_cc+pg)*1000)/29.97);	
	if (extraction_start.set && extraction_start.time_in_ms>ms_start) 
		return 0;
	ms_start+=subs_delay;
	if (ms_start<0) // Drop screens that because of subs_delay start too early
		return 0;
	ms_end = (long) (((totalblockswritten_thisfile()+pg)*1000)/29.97)+subs_delay;		
    mstotime (ms_start,&h1,&m1,&s1,&ms1);
    mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	wb->data608->srt_counter++;
    sprintf (timeline,"%u\r\n",wb->data608->srt_counter);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);		
    sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
        h1,m1,s1,ms1, h2,m2,s2,ms2);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
    if (debug_608)
    {
        printf ("\r");
        printf ("%s", timeline);
    }
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);		
    
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {	
			int length;
			if (sentence_cap)
			{
				capitalize (i,data);
				correct_case(i,data);
			}
            length = get_decoder_line_encoded (subline, i, data);
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                printf ("\r");
                printf ("%s\n",subline);
            }
            fwrite (subline, 1, length, wb->fh);
            fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
			wrote_something=1;
            // fprintf (wb->fh,encoded_crlf);
        }
    }
    if (debug_608)
    {
        printf ("\r\n");
    }
    // fprintf (wb->fh, encoded_crlf);
    fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
	return wrote_something;
}

int write_cc_buffer_as_sami (struct eia608_screen *data, struct s_write *wb)
{
	int i;
	int wrote_something=0;
    unsigned pg = c1global? c1global:c2global; // Any non-zero global will do
    LONG startms = (LONG) (((wb->data608->current_visible_start_cc+pg)*1000)/29.97);
    LONG endms;
	if (extraction_start.set && extraction_start.time_in_ms>startms) 
		return 0;
	startms+=subs_delay;
	if (startms<0) // Drop screens that because of subs_delay start too early
		return 0; 
	endms = (LONG) (((totalblockswritten_thisfile()+pg)*1000)/29.97)+subs_delay;
    endms--; // To prevent overlapping with next line.
    sprintf ((char *) str,"<SYNC start=\"%ld\"><P class=\"UNKNOWNCC\">\r\n",startms);
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        printf ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);		
    for (i=0;i<15;i++)
    {
        if (data->row_used[i])
        {				
            int length = get_decoder_line_encoded (subline, i, data);
            if (debug_608 && encoding!=ENC_UNICODE)
            {
                printf ("\r");
                printf ("%s\n",subline);
            }
            fwrite (subline, 1, length, wb->fh);
			wrote_something=1;
            if (i!=14)
                fwrite (encoded_br, 1, encoded_br_length,wb->fh);			
            fwrite (encoded_crlf, 1, encoded_crlf_length,wb->fh);
        }
    }
    sprintf ((char *) str,"</P></SYNC>\r\n");
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        printf ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
    sprintf ((char *) str,"<SYNC start=\"%ld\"><P class=\"UNKNOWNCC\">&nbsp</P></SYNC>\r\n\r\n",endms);
    if (debug_608 && encoding!=ENC_UNICODE)
    {
        printf ("\r%s\n", str);
    }
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
    fwrite (enc_buffer,enc_buffer_used,1,wb->fh);
	return wrote_something;
}

int write_cc_buffer (struct s_write *wb)
{
    struct eia608_screen *data;
	int wrote_something=0;
	if (screens_to_process!=-1 && wb->data608->screenfuls_counter>=screens_to_process)
	{
		// We are done. 
		processed_enough=1;
		return 0;
	}
    if (wb->data608->visible_buffer==1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;

    if (!data->empty)
    {
		new_sentence=1;
        switch (write_format)
        {
            case OF_SRT:
                wrote_something = write_cc_buffer_as_srt (data, wb);
                break;
            case OF_SAMI:
                wrote_something = write_cc_buffer_as_sami (data,wb);
                break;
        }
    }
	return wrote_something;
}

void roll_up(struct s_write *wb)
{
    int i;
    int keep_lines;
    int lastrow;
    int rows_now=0;
    eia608_screen *use_buffer;
    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;
    switch (wb->data608->mode)
    {
    case MODE_ROLLUP_2:
        keep_lines=2;
        break;
    case MODE_ROLLUP_3:
        keep_lines=3;
        break;
    case MODE_ROLLUP_4:
        keep_lines=4;
        break;
    default: // Shouldn't happen
        keep_lines=0;
        break;
    }

    // Look for the last line used
    for ( i=0;i<15;i++)
        if (use_buffer->row_used[i])
            rows_now++;
    if (rows_now>keep_lines)
        printf ("Bug here.\n");


    for (i=14;i>=0 && use_buffer->row_used[i]==0;i--) {}
    if (i>0)
    {
		int j;
        lastrow=i;
        for (j=lastrow-keep_lines+1;j<lastrow; j++)
        {
            if (j>=0)
            {
                memcpy (use_buffer->characters[j],use_buffer->characters[j+1],33);
                memcpy (use_buffer->colors[j],use_buffer->colors[j+1],33);
                memcpy (use_buffer->fonts[j],use_buffer->fonts[j+1],33);
                use_buffer->row_used[j]=use_buffer->row_used[j+1];
            }
        }
        for (j=0;j<(1+wb->data608->cursor_row-keep_lines);j++)
        {
            memset(use_buffer->characters[j],' ',32);			
            memset(use_buffer->colors[j],COL_WHITE,32);
            memset(use_buffer->fonts[j],FONT_REGULAR,32);
            use_buffer->characters[j][32]=0;
            use_buffer->row_used[j]=0;
        }
        memset(use_buffer->characters[lastrow],' ',32);
        memset(use_buffer->colors[lastrow],COL_WHITE,32);
        memset(use_buffer->fonts[lastrow],FONT_REGULAR,32);

        use_buffer->characters[lastrow][32]=0;
        use_buffer->row_used[lastrow]=0;
    }
    rows_now=0;
    for (i=0;i<15;i++)
        if (use_buffer->row_used[i])
            rows_now++;
    if (rows_now>keep_lines)
        printf ("Bug here.\n");
}

void erase_memory (struct s_write *wb, int displayed)
{
    eia608_screen *buf;
    if (displayed)
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer1;
        else
            buf=&wb->data608->buffer2;
    }
    else
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer2;
        else
            buf=&wb->data608->buffer1;
    }
    clear_eia608_cc_buffer (buf);
}

void handle_command (/*const */ unsigned char c1, const unsigned char c2, struct s_write *wb)
{
    int command = COM_UNKNOWN;
    if (c1==0x15)
        c1=0x14;
    if ((c1==0x14 || c1==0x1C) && c2==0x2C)
        command = COM_ERASEDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x20)
        command = COM_RESUMECAPTIONLOADING;
    if ((c1==0x14 || c1==0x1C) && c2==0x2F)
        command = COM_ENDOFCAPTION;
    if ((c1==0x17 || c1==0x1F) && c2==0x21)
        command = COM_TABOFFSET1;
    if ((c1==0x17 || c1==0x1F) && c2==0x22)
        command = COM_TABOFFSET2;
    if ((c1==0x17 || c1==0x1F) && c2==0x23)
        command = COM_TABOFFSET3;
    if ((c1==0x14 || c1==0x1C) && c2==0x25)
        command = COM_ROLLUP2;
    if ((c1==0x14 || c1==0x1C) && c2==0x26)
        command = COM_ROLLUP3;
    if ((c1==0x14 || c1==0x1C) && c2==0x27)
        command = COM_ROLLUP4;
    if ((c1==0x14 || c1==0x1C) && c2==0x2D)
        command = COM_CARRIAGERETURN;
    if ((c1==0x14 || c1==0x1C) && c2==0x2E)
        command = COM_ERASENONDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x21)
        command = COM_BACKSPACE;
	if ((c1==0x14 || c1==0x1C) && c2==0x2b)
        command = COM_RESUMETEXTDISPLAY;
    if (debug_608)
    {
        printf ("\rCommand: %02X %02X (%s)\n",c1,c2,command_type[command]);
    }
    switch (command)
    {
    case COM_BACKSPACE:
        if (wb->data608->cursor_column>0)
        {
            wb->data608->cursor_column--;
            get_writing_buffer(wb)->characters[wb->data608->cursor_row][wb->data608->cursor_column]=' ';
        }
        break;
    case COM_TABOFFSET1:
        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
        break;
    case COM_TABOFFSET2:
        wb->data608->cursor_column+=2;
        if (wb->data608->cursor_column>31)
            wb->data608->cursor_column=31;
        break;
    case COM_TABOFFSET3:
        wb->data608->cursor_column+=3;
        if (wb->data608->cursor_column>31)
            wb->data608->cursor_column=31;
        break;
    case COM_RESUMECAPTIONLOADING:
        wb->data608->mode=MODE_POPUP;
        break;
	case COM_RESUMETEXTDISPLAY:
		wb->data608->mode=MODE_TEXT;
		break;
    case COM_ROLLUP2:
        wb->data608->mode=MODE_ROLLUP_2;
        if (wb->data608->mode==MODE_POPUP)
		{
			if (write_cc_buffer (wb))
				wb->data608->screenfuls_counter++;
            erase_memory (wb, true);			
		}
        erase_memory (wb, false);
        wb->data608->cursor_column=0;
        wb->data608->cursor_row=wb->data608->rollup_base_row;
        break;
    case COM_ROLLUP3:
        if (wb->data608->mode==MODE_POPUP)
		{
			if (write_cc_buffer (wb))
				wb->data608->screenfuls_counter++;
            erase_memory (wb, true);
			
		}
        wb->data608->mode=MODE_ROLLUP_3;
        erase_memory (wb, false);
        wb->data608->cursor_column=0;
        wb->data608->cursor_row=wb->data608->rollup_base_row;
        break;
    case COM_ROLLUP4:
        if (wb->data608->mode==MODE_POPUP)
		{
			if (write_cc_buffer (wb))
				wb->data608->screenfuls_counter++;
            erase_memory (wb, true);			
		}
        wb->data608->mode=MODE_ROLLUP_4;
        wb->data608->cursor_column=0;
        wb->data608->cursor_row=wb->data608->rollup_base_row;
        erase_memory (wb, false);
        break;
    case COM_CARRIAGERETURN:
        if (write_cc_buffer(wb))
			wb->data608->screenfuls_counter++;
        roll_up(wb);		
        wb->data608->current_visible_start_cc=totalblockswritten_thisfile();
        wb->data608->cursor_column=0;
        break;
    case COM_ERASENONDISPLAYEDMEMORY:
        erase_memory (wb,false);
        break;
    case COM_ERASEDISPLAYEDMEMORY:
        // Write it to disk before doing this, and make a note of the new
        // time it became clear.
        if (write_cc_buffer (wb))
			wb->data608->screenfuls_counter++;
        erase_memory (wb,true);
        wb->data608->current_visible_start_cc=totalblockswritten_thisfile();
        break;
    case COM_ENDOFCAPTION: // Switch buffers
        // The currently *visible* buffer is leaving, so now we know it's ending
        // time. Time to actually write it to file.
        if (write_cc_buffer (wb))
			wb->data608->screenfuls_counter++;
        wb->data608->visible_buffer = (wb->data608->visible_buffer==1) ? 2 : 1;
        wb->data608->current_visible_start_cc=totalblockswritten_thisfile();
        wb->data608->cursor_column=0;
        wb->data608->cursor_row=0;
		wb->data608->color=default_color;
        wb->data608->font=FONT_REGULAR;
        break;
    default:
        if (debug_608)
        {
            printf ("\rNot yet implemented.\n");
        }
        break;
    }
}


void handle_double (const unsigned char c1, const unsigned char c2, struct s_write *wb)
{
    unsigned char c;
    if (wb->data608->channel!=cc_channel)
        return;
    if (c2>=0x30 && c2<=0x3f)
    {
        c=c2 + 0x50; // So if c>=0x80 && c<=0x8f, it comes from here
        if (debug_608)
            printf ("\rDouble: %02X %02X  -->  %c\n",c1,c2,c);
        write_char(c,wb);
    }
}

unsigned char handle_extended (unsigned char hi, unsigned char lo, struct s_write *wb)
{
    // For lo values between 0x20-0x3f
    unsigned char c=0;

    if (debug_608)
        printf ("\rExtended: %02X %02X\n",hi,lo);
    if (lo>=0x20 && lo<=0x3f && (hi==0x12 || hi==0x13))
    {
        switch (hi)
        {
        case 0x12:
            c=lo+0x70; // So if c>=0x90 && c<=0xaf it comes from here
            break;
        case 0x13:
            c=lo+0x90; // So if c>=0xb0 && c<=0xcf it comes from here
            break;
        }
		// This column change is because extended characters replace 
		// the previous character (which is sent for basic decoders
		// to show something similar to the real char)
		if (wb->data608->cursor_column>0)        
            wb->data608->cursor_column--;        
        

        write_char (c,wb);
    }
    return c;
}

void handle_pac (unsigned char c1, unsigned char c2, struct s_write *wb)
{
    int color;
    int font;
    int indent;
    /*if (wb->data608->channel!=cc_channel)
    return; */

    int row=rowdata[((c1<<1)&14)|((c2>>5)&1)];

    // int base_row = pac1_to_row[c1-0x11];
    // int row = -1;

    if (debug_608)
        printf ("\rPAC: %02X %02X\n",c1,c2);

    if (c2>=0x40 && c2<=0x5f)
    {
        c2=c2-0x40;
    }
    else
    {
        if (c2>=0x60 && c2<=0x7f)
        {
            c2=c2-0x60;
        }
        else
        {
            if (debug_608)
                printf ("\rThis is not a PAC!!!!!\n");
            return;
        }
    }
    color=pac2_attribs[c2][0];
    font=pac2_attribs[c2][1];
    indent=pac2_attribs[c2][2];
    if (debug_608)
        printf ("\rPosition: %d:%d, color: %s,  font: %s\n",row,
        indent,color_text[color][0],font_text[font]);
	if (wb->data608->mode!=MODE_TEXT)
	{
		// According to Robson, row info is discarded in text mode
		// but column is accepted
		wb->data608->cursor_row=row-1 ; // Since the array is 0 based
	}
    wb->data608->rollup_base_row=row-1;
    wb->data608->cursor_column=indent;	
}


void handle_single (const unsigned char c1, struct s_write *wb)
{	
    if (c1<0x20 || wb->data608->channel!=cc_channel)
        return; // We don't allow special stuff here
     /*if (debug_608)
        printf ("Caracter: %02X (%c) -> %02X (%c)\n",c1,c1,c,c); */
    write_char (c1,wb);
#ifndef undef
//	dump_data(&c1,1);
#endif
}

void check_channel (unsigned char c1, struct s_write *wb)
{
    if (wb->data608->channel!=1 && c1==0x14)
    {
        if (debug_608)
            printf ("\rChannel change, now 1\n");
        wb->data608->channel=1;
    }
    if (wb->data608->channel!=2 && c1==0x1c)
    {
        if (debug_608)
            printf ("\rChannel change, now 2\n");
        wb->data608->channel=2;
    }
    if ((wb->data608->channel!=3) && (((c1>=0x01) &&
        (c1<=0x0f)) || (c1==0x15)))
    {
        if (debug_608)
            printf ("\rChannel change, now 3\n");
        wb->data608->channel=3;
    }
    if (wb->data608->channel!=4 && c1==0x1d)
    {
        if (debug_608)
            printf ("\rChannel change, now 4\n");
        wb->data608->channel=4;
    }
}

/* Returns 1 if something was written to screen, 0 otherwise */
int disCommand (unsigned char hi, unsigned char lo, struct s_write *wb)
{
	int wrote_to_screen=0;

    if (hi>=0x18 && hi<=0x1f)
        hi=hi-8;
    switch (hi)
    {
    case 0x10:
        if (lo>=0x40 && lo<=0x5f)
            handle_pac (hi,lo,wb);
        break;
    case 0x11:
        if (lo>=0x20 && lo<=0x2f)
            handle_text_attr (hi,lo,wb);
        if (lo>=0x30 && lo<=0x3f)
		{
			wrote_to_screen=1;
            handle_double (hi,lo,wb);
		}
        if (lo>=0x40 && lo<=0x7f)
            handle_pac (hi,lo,wb);
        break;
    case 0x12:
    case 0x13:
        if (lo>=0x20 && lo<=0x3f)
		{
            handle_extended (hi,lo,wb);
			wrote_to_screen=1;
		}
        if (lo>=0x40 && lo<=0x7f)
            handle_pac (hi,lo,wb);
        break;
    case 0x14:
    case 0x15:
        if (lo>=0x20 && lo<=0x2f)
            handle_command (hi,lo,wb);
        if (lo>=0x40 && lo<=0x7f)
            handle_pac (hi,lo,wb);
        break;
    case 0x16:
        if (lo>=0x40 && lo<=0x7f)
            handle_pac (hi,lo,wb);
        break;
    case 0x17:
        if (lo>=0x21 && lo<=0x22)
            handle_command (hi,lo,wb);
        if (lo>=0x2e && lo<=0x2f)
            handle_text_attr (hi,lo,wb);
        if (lo>=0x40 && lo<=0x7f)
            handle_pac (hi,lo,wb);
        break;
    }
	return wrote_to_screen;
}

void process608 (const unsigned char *data, int length, struct s_write *wb)
{
    if (data!=NULL)
    {
		int i;
        for (i=0;i<length;i=i+2)
        {
            unsigned char hi, lo;
			int wrote_to_screen=0; 
            hi = data[i] & 0x7F; // Get rid of parity bit
            lo = data[i+1] & 0x7F; // Get rid of parity bit

            if (hi==0 && lo==0) // Just padding
                continue;
            // printf ("\r[%02X:%02X]\n",hi,lo);
            check_channel (hi,wb);
            if (wb->data608->channel!=cc_channel)
                continue;
            if (hi>=0x01 && hi<=0x0E)
            {
                // XDS crap - mode. Would be nice to support it eventually
                // wb->data608->last_c1=0;
                // wb->data608->last_c2=0;
                in_xds_mode=1;
            }
            if (hi==0x0F) // End of XDS block
            {
                in_xds_mode=0;
                continue;
            }
            if (hi>=0x10 && hi<0x1F) // Back to normal
            {
                in_xds_mode=0;
                if (wb->data608->last_c1==hi && wb->data608->last_c2==lo)
                {
                    // Duplicate dual code, discard
                    continue;
                }
                wb->data608->last_c1=hi;
                wb->data608->last_c2=lo;
                wrote_to_screen=disCommand (hi,lo,wb);
            }
            if (hi>=0x20)
            {
                handle_single(hi,wb);
                handle_single(lo,wb);
				wrote_to_screen=1;
                wb->data608->last_c1=0;
                wb->data608->last_c2=0;
            }
			if (wrote_to_screen && direct_rollup && 
				(wb->data608->mode==MODE_ROLLUP_2 ||
				wb->data608->mode==MODE_ROLLUP_3 ||
				wb->data608->mode==MODE_ROLLUP_4))
			{
				// We don't increase screenfuls_counter here.
				write_cc_buffer (wb);
				wb->data608->current_visible_start_cc=totalblockswritten_thisfile();
			}
        }
    }
}

