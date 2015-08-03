#ifndef _VO_H
#define _VO_H
void vo_init(int width, int height, char *title);
void vo_draw(unsigned char * buf);
void vo_refresh();
void vo_wait();
void vo_close();
void ShowHelp(char **ta);
void ShowDetails(char *t);
#endif
