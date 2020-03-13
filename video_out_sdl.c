#ifndef _WIN32
/*
 * video_out_sdl.c
 *
 * Copyright (C) 2000-2003 Ryan C. Gordon <icculus@lokigames.com> and
 *                         Dominik Schnitzer <aeneas@linuxvideo.org>
 *
 * SDL info, source, and binaries can be found at http://www.libsdl.org/
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//#include "config.h"

#define LIBVO_SDL
#define vo_instance_t int

typedef struct {
     int dummy;
} vo_setup_result_t;

extern int key;
extern int xPos,yPos,lMouseDown;
extern char osname[1024];

#ifdef LIBVO_SDL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
//#include <inttypes.h>
//#include "video_out.h"

typedef struct {
    vo_instance_t vo;
    int width;
    int height;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint32 sdlflags;
    Uint8 bpp;
} sdl_instance_t;

static int sdl_setup (vo_instance_t * _instance, unsigned int width,
          unsigned int height, unsigned int chroma_width,
          unsigned int chroma_height, vo_setup_result_t * result)
{
    sdl_instance_t * instance = (sdl_instance_t *) _instance;

    instance->width = width;
    instance->height = height;

    instance->window = SDL_CreateWindow("comskip",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      width, height,
      SDL_WINDOW_RESIZABLE);
    if (! (instance->window)) {
      fprintf(stderr, "sdl could not setup a window\n");
      return 1;
    }

    instance->renderer = SDL_CreateRenderer(instance->window, -1, 0);
    if (! (instance->renderer)) {
      fprintf(stderr, "sdl could not create renderer\n");
      return 1;
    }

    instance->texture = SDL_CreateTexture(instance->renderer,
      SDL_PIXELFORMAT_RGB24,
      SDL_TEXTUREACCESS_STREAMING,
      width, height);
    if (! (instance->texture)) {
      fprintf(stderr, "sdl could not create texture\n");
      return 1;
    }

    SDL_SetRenderDrawColor(instance->renderer, 0, 0, 0, 255);
    SDL_RenderClear(instance->renderer);
    SDL_RenderPresent(instance->renderer);

    //result->convert = NULL;
    return 0;
}

vo_instance_t * vo_sdl_open (void)
{
    sdl_instance_t * instance;

    instance = (sdl_instance_t *) malloc (sizeof (sdl_instance_t));
    if (instance == NULL)
      return NULL;

    memset (instance, 0, sizeof (sdl_instance_t));

    if (SDL_Init (SDL_INIT_VIDEO)) {
      fprintf(stderr, "sdl video initialization failed.\n");
      return NULL;
    }

    return (vo_instance_t *) instance;
}
#endif

void ShowHelp(char **ta)
{
  char *t;
  while ((t = *ta)) {
    printf("%s\n", t);
    ta++;
  }
}

void ShowDetails(char *t)
{
  printf("%s\n", t);
}

void handle_event(SDL_Event event) {
  static int drag = 0;

  switch (event.type) {
    case SDL_QUIT:
      //printf("quit!\n");
      exit(0);
      break;

    case SDL_MOUSEBUTTONDOWN: {
      //printf("mousedown: (%d,%d) (%d)\n", event.button.x, event.button.y, event.button.state);
      xPos = event.button.x;
      yPos = event.button.y;
      lMouseDown = 1;
      drag = 1;
      break;
    }

    case SDL_MOUSEBUTTONUP:
      drag = 0;
      break;

    case SDL_MOUSEMOTION:
      if (drag) {
        xPos = event.button.x;
        yPos = event.button.y;
        lMouseDown = 1;
      }
      break;

    case SDL_KEYDOWN: {
      SDL_Keysym ks = event.key.keysym;
      //printf("key: %d (%c) [scan: %d (%c)]\n", ks.sym, ks.sym, ks.scancode, ks.scancode);
      switch (ks.sym) {
        case SDLK_ESCAPE:
#ifdef TEST
          exit(0);
#else
          key = event.key.keysym.sym;
#endif
          break;
        case SDLK_SPACE:
          key = 0x20;
          break;
        case SDLK_F1:
          key = 112;
          break;
        case SDLK_F2:
          key = 113;
          break;
        case SDLK_F3:
          key = 114;
          break;
        case SDLK_F4:
          key = 115;
          break;
        case SDLK_F5:
          key = 116;
          break;
        case SDLK_PERIOD:
          key = '.';
          break;
        case SDLK_LEFT:
          if (ks.mod & KMOD_SHIFT)
            key = 80;
          else
            key = 37;
          break;
        case SDLK_RIGHT:
          if (ks.mod & KMOD_SHIFT)
            key = 78;
          else
            key = 39;
          break;
        case SDLK_UP:
          key = 38;
          break;
        case SDLK_DOWN:
          key = 40;
          break;
        case SDLK_PAGEUP:
          if (ks.mod & KMOD_RALT || ks.mod & KMOD_LALT) {
            key = 133;
          } else {
            key = 33;
          }
          break;
        case SDLK_PAGEDOWN:
          if (ks.mod & KMOD_RALT || ks.mod & KMOD_LALT) {
            key = 134;
          } else {
            key = 34;
          }
          break;
        default:
          if (ks.sym >= SDLK_a && ks.sym <= SDLK_z) {
            key = ks.sym - 32;
            //printf("PRESSED: %c (%d)\n", key, key);
          }
          break;
      }
      break;
    }
  }
}

#define MAXHEIGHT	1200
#define MAXWIDTH	2000

unsigned char buf0[MAXWIDTH*MAXHEIGHT*3];

vo_instance_t * instance;
vo_setup_result_t result;

void vo_init(int width, int height, char *title)
{
  instance = vo_sdl_open();
  sdl_setup(instance, width, height, width, height, &result);
}

void vo_draw(unsigned char * buf)
{
  sdl_instance_t *sdl = (sdl_instance_t *) instance;
  SDL_Event event;
  while (SDL_PollEvent (&event)) {
    handle_event(event);
  }

  SDL_UpdateTexture(sdl->texture, NULL, buf, sdl->width * 3);
  SDL_RenderClear(sdl->renderer);
  SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
  SDL_RenderPresent(sdl->renderer);
}

void vo_refresh()
{
  if (!instance) return;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    handle_event(event);
  }
}

void vo_wait()
{
  if (!instance) return;
  SDL_Event event;
  if (SDL_WaitEvent(&event)) {
    handle_event(event);
  }
}

void vo_close()
{
  SDL_Quit();
}
#endif


#ifdef TEST

int main(int argc, char **argv)
{
     int r,g,b;

     /*
     	double  R,G,B;
     	double Y,U,V;

     	R = 0;
     	G = 0;
     	B = 0;

     	Y 	= 0.299*R + 0.587*G + 0.114*B;
     	U 	= - 0.147*R - 0.289*G + 0.436*B;
     	V 	= 0.615*R - 0.515*G - 0.100*B;


     	R = 1;
     	G = 1;
     	B = 1;

     	Y 	= 0.299*R + 0.587*G + 0.114*B;
     	U 	= - 0.147*R - 0.289*G + 0.436*B;
     	V 	= 0.615*R - 0.515*G - 0.100*B;
     */

     int width = 640;
     int height = 480;
     vo_init(width, height, "test");

     memset(buf0,0,sizeof(buf0));

     for (r = 0 ; r < 255; r++) {
          for (g = 0; g < 255; g++) {
               for (b=0; b<255; b++) {
                    int x = b, y = g;
                    buf0[3*(y*width+x)] = b;
                    buf0[3*(y*width+x)+1] = g;
                    buf0[3*(y*width+x)+2] = r;
               }
          }
          vo_draw(buf0);
     }

     while (1) vo_wait();

     return 0;
}

#endif
