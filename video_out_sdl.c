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
#include <SDL.h>
//#include <inttypes.h>
//#include "video_out.h"

typedef struct {
    vo_instance_t vo;
    int width;
    int height;
    SDL_Surface * surface;
    SDL_Surface * rgb;
    Uint32 sdlflags;
    Uint8 bpp;
} sdl_instance_t;

#if 0
static void sdl_setup_fbuf (vo_instance_t * _instance,
          uint8_t ** buf, void ** id)
{
    sdl_instance_t * instance = (sdl_instance_t *) _instance;
    SDL_Overlay * overlay;

    *id = overlay = SDL_CreateYUVOverlay (instance->width, instance->height,
            SDL_YV12_OVERLAY, instance->surface);

    if (!buf) return;
/*
    buf[0] = overlay->pixels[0];
    buf[1] = overlay->pixels[2];
    buf[2] = overlay->pixels[1];
    if (((long)buf[0] & 15) || ((long)buf[1] & 15) || ((long)buf[2] & 15)) {
  fprintf (stderr, "Unaligned buffers. Anyone know how to fix this ?\n");
  exit (1);
    }
*/
}

static void sdl_start_fbuf (vo_instance_t * instance,
          uint8_t * const * buf, void * id)
{
    SDL_LockYUVOverlay ((SDL_Overlay *) id);
}

static void sdl_draw_frame (vo_instance_t * _instance,
          uint8_t * const * buf, void * id)
{
    sdl_instance_t * instance = (sdl_instance_t *) _instance;
    SDL_Overlay * overlay = (SDL_Overlay *) id;
    SDL_Event event;

    while (SDL_PollEvent (&event))
  if (event.type == SDL_VIDEORESIZE)
      instance->surface =
    SDL_SetVideoMode (event.resize.w, event.resize.h,
          instance->bpp, instance->sdlflags);
    SDL_DisplayYUVOverlay (overlay, &(instance->surface->clip_rect));
}

static void sdl_discard (vo_instance_t * _instance,
       uint8_t * const * buf, void * id)
{
    SDL_UnlockYUVOverlay ((SDL_Overlay *) id);
}

static void sdl_close (vo_instance_t * _instance)
{
    sdl_instance_t * instance;
    int i;

    instance = (sdl_instance_t *) _instance;
    for (i = 0; i < 3; i++)
  SDL_FreeYUVOverlay (instance->frame[i].overlay);
    SDL_FreeSurface (instance->surface);
    SDL_QuitSubSystem (SDL_INIT_VIDEO);
}
#endif

static int sdl_setup (vo_instance_t * _instance, unsigned int width,
          unsigned int height, unsigned int chroma_width,
          unsigned int chroma_height, vo_setup_result_t * result)
{
    sdl_instance_t * instance = (sdl_instance_t *) _instance;

    instance->width = width;
    instance->height = height;
    instance->surface = SDL_SetVideoMode (width, height, instance->bpp,
            instance->sdlflags);
    if (! (instance->surface)) {
  fprintf (stderr, "sdl could not set the desired video mode\n");
  return 1;
    }
    instance->rgb = SDL_CreateRGBSurface(SDL_SRCCOLORKEY, width, height, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
    if (! (instance->rgb)) {
  fprintf (stderr, "sdl could not create RGB surface\n");
  return 1;
    }

    //result->convert = NULL;
    return 0;
}

vo_instance_t * vo_sdl_open (void)
{
    sdl_instance_t * instance;
    const SDL_VideoInfo * vidInfo;

    instance = (sdl_instance_t *) malloc (sizeof (sdl_instance_t));
    if (instance == NULL)
  return NULL;

    memset (instance, 0, sizeof (sdl_instance_t));
    //instance->vo.setup = sdl_setup;
    //instance->vo.setup_fbuf = sdl_setup_fbuf;
    //instance->vo.set_fbuf = NULL;
    //instance->vo.start_fbuf = sdl_start_fbuf;
    //instance->vo.discard = sdl_discard;
    //instance->vo.draw = sdl_draw_frame;
    //instance->vo.close = NULL; /* sdl_close; */
    instance->sdlflags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL;

    //putenv("SDL_VIDEO_YUV_HWACCEL=1");
    //putenv("SDL_VIDEO_X11_NODIRECTCOLOR=1");

    if (SDL_Init (SDL_INIT_VIDEO)) {
  fprintf (stderr, "sdl video initialization failed.\n");
  return NULL;
    }

    vidInfo = SDL_GetVideoInfo ();
    if (!SDL_ListModes (vidInfo->vfmt, SDL_HWSURFACE | SDL_RESIZABLE)) {
  instance->sdlflags |= SDL_RESIZABLE;
  if (!SDL_ListModes (vidInfo->vfmt, SDL_RESIZABLE)) {
      fprintf (stderr, "sdl couldn't get any acceptable video mode\n");
      return NULL;
  }
    }
    instance->bpp = vidInfo->vfmt->BitsPerPixel;
    if (instance->bpp < 16) {
  fprintf(stderr, "sdl has to emulate a 16 bit surfaces, "
    "that will slow things down.\n");
  instance->bpp = 16;
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
      SDL_keysym ks = event.key.keysym;
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
  SDL_WM_SetCaption(title, "comskip");
  sdl_setup(instance, width, height, width, height, &result);
  SDL_EnableKeyRepeat(50, SDL_DEFAULT_REPEAT_INTERVAL);
}

void vo_draw(unsigned char * buf)
{
  sdl_instance_t *sdl_instance = (sdl_instance_t *) instance;
  SDL_Event event;
  while (SDL_PollEvent (&event)) {
    handle_event(event);
  }

  SDL_Surface *rgb = sdl_instance->rgb;
  SDL_LockSurface( rgb );
  memcpy(rgb->pixels, buf, rgb->w * rgb->h * rgb->format->BytesPerPixel);
  SDL_UnlockSurface( rgb );

  SDL_Surface *screen = sdl_instance->surface;
  SDL_BlitSurface(rgb, NULL, screen, NULL);
  SDL_Flip(screen);
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
     //if (instance) sdl_close(instance);
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

     return 0;
}

#endif
