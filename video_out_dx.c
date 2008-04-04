#ifdef _WIN32
/*
 * video_out_dx.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 *
 * Contributed by Gildas Bazin <gbazin@netcourrier.com>
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

#include "resource.h"



#define LIBVO_DX
#define uint8_t	unsigned char
#define vo_instance_t int


typedef struct {
	int dummy;
} vo_setup_result_t;


int key;
char osname[1024];


#ifdef LIBVO_DX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <inttypes.h>

//#include "mpeg2.h"
//#include "video_out.h"
//#include "mpeg2convert.h"

#include <ddraw.h>
#include <initguid.h>


static HDC hdc;
static BITMAPINFOHEADER birgb;
static LPBITMAPINFOHEADER lpbirgb = &birgb;
static PAINTSTRUCT ps;

int xPos,yPos,lMouseDown;


#define USE_OVERLAY_TRIPLE_BUFFERING 0

/*
 * DirectDraw GUIDs.
 * Defining them here allows us to get rid of the dxguid library during link.
 */
DEFINE_GUID (IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID (IID_IDirectDrawSurface2, 0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27);

#define FOURCC_YV12 0x32315659

HINSTANCE hInst;
HWND hWind;


typedef struct {
//    vo_instance_t vo;
    int width;
    int height;

    HWND window;
    RECT window_coords;
    HINSTANCE hddraw_dll;
    LPDIRECTDRAW2 ddraw;
    LPDIRECTDRAWSURFACE2 display;
    LPDIRECTDRAWCLIPPER clipper;
    LPDIRECTDRAWSURFACE2 frame[3];
    int index;

    LPDIRECTDRAWSURFACE2 overlay;
    uint8_t * yuv[3];
    int stride;
	char title[256];
} dx_instance_t;


static void * surface_addr (LPDIRECTDRAWSURFACE2 surface, int * stride);
static LPDIRECTDRAWSURFACE2 alloc_overlay (dx_instance_t * instance);
static void dx_draw_frame (vo_instance_t * _instance, uint8_t * const * buf, void * id);

#define OPEN_INPUT	1
#define OPEN_INI	2
#define SAVE_DMP	3
#define SAVE_INI	4

int PopFileDlg(HWND hWind, PTSTR pstrFileName, int Status)
{
	static OPENFILENAME ofn;
	static char *szFilter, *ext;

	switch (Status)
	{
		case OPEN_INPUT:
			ofn.nFilterIndex = 1;
			szFilter = \
				TEXT ("mpg, mpeg, m2v, mpv\0*.mpg;*.mpeg;*.m2v;*.mpv\0") \
				TEXT ("tp, ts\0*.tp;*.ts\0") \
				TEXT ("dvr-ms\0*.dvr-ms\0") \
				TEXT ("All MPEG Files\0*.vob;*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv;*.tp;*.ts;*.trp;*.pva;*.vro\0") \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case SAVE_DMP:
			szFilter = TEXT ("DMP File (*.dmp)\0*.dmp\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_INI:
		case SAVE_INI:
			szFilter = TEXT ("Comskip INI File (*.d2v)\0*.d2v\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

	}

	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hWind;
	ofn.hInstance         = hInst ;
	ofn.lpstrFilter       = szFilter ;
	ofn.nMaxFile          = 1024 ;
	ofn.nMaxFileTitle     = 1024 ;
	ofn.lpstrFile         = pstrFileName ;
	ofn.lpstrInitialDir   = TEXT(".");

	switch (Status)
	{
		case OPEN_INPUT:
		case OPEN_INI:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
			return GetOpenFileName(&ofn);

		case SAVE_DMP:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".dmp", 4))
					*ext = 0;
				return 1;
			}
			break;

		case SAVE_INI:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".ini", 4))
					*ext = 0;
				return 1;
			}
			break;
	}
	return 0;
}




static void update_overlay (dx_instance_t * instance)
{
    DDOVERLAYFX ddofx;
    DWORD dwFlags;

    memset (&ddofx, 0, sizeof (DDOVERLAYFX));
    ddofx.dwSize = sizeof (DDOVERLAYFX);
    dwFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;
    IDirectDrawSurface2_UpdateOverlay (instance->overlay, NULL,
				       instance->display,
				       &instance->window_coords,
				       dwFlags, &ddofx);
}

static int drag = 0;


static HANDLE hThread;
static DWORD threadId;


// Mesage handler for about box.
static long FAR PASCAL About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}



static long FAR PASCAL event_procedure (HWND hwnd, UINT message,
					WPARAM wParam, LPARAM lParam)
{
    RECT rect_window;
    POINT point_window;
    dx_instance_t * instance;
	short wmId, wmEvent;

    switch (message) {
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox (hInst, (LPCTSTR)IDD_ABOUT, hwnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			PostQuitMessage(0);
			exit(1);
			break;
		default:
		    return DefWindowProc (hwnd, message, wParam, lParam);
		}
		break;
		
		case WM_WINDOWPOSCHANGED:
			instance = (dx_instance_t *) GetWindowLong (hwnd, GWL_USERDATA);
			
			/* update the window position and size */
			point_window.x = 0;
			point_window.y = 0;
			ClientToScreen (hwnd, &point_window);
			instance->window_coords.left = point_window.x;
			instance->window_coords.top = point_window.y;
			GetClientRect (hwnd, &rect_window);
			instance->window_coords.right = rect_window.right + point_window.x;
			instance->window_coords.bottom = rect_window.bottom + point_window.y;
			
			/* update the overlay */
			//	if (instance->overlay && instance->display)
			//	    update_overlay (instance);
			
			//	return 0;
			
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			ReleaseDC(hwnd, hdc);
			
			break;
			
			
		case WM_CLOSE:	
			//			ReleaseDC(hWnd, hDC);
			//			DeleteObject(hBrush);
			
			PostQuitMessage(0);
			break;
			
			return 0;
			
		case WM_DESTROY:	/* just destroy the window */
			PostQuitMessage (0);
			return 0;
			
		case WM_LBUTTONDOWN:
			if (lParam > -1)
			{
				xPos = (int)(LOWORD(lParam));
				yPos = (int)(HIWORD(lParam));
				lMouseDown = 1;
				drag = 1;
			}
			break;
		case WM_MOUSEMOVE:
			if (lParam > -1 && drag)
			{
				xPos = (int)(LOWORD(lParam));
				yPos = (int)(HIWORD(lParam));
				lMouseDown = 1;
				
			}
			break;
		case WM_LBUTTONUP:
			if (lParam > -1)
			{
				drag = 0;
				
			}
			break;
			
			
		case WM_KEYDOWN:
			
			switch (wParam)
			{
			case VK_DOWN:
				key = 1;						
				break;
				
			case VK_UP:
				key = 2;						
				break;
				
			case VK_LEFT:
				key = 3;						
				break;
				
			case VK_RIGHT:
				key = 4;						
				break;
				
			case VK_NEXT:
				key = 5;						
				break;
				
			case VK_PRIOR:
				key = 6;						
				break;
			}
			if (wParam != 0) {
				key = wParam;
				if (key == 'C')
					if(PopFileDlg(hwnd, osname, SAVE_DMP) == 0)
						key = 0;
			}
			break;
	}
	/*
	if (key || lMouseDown) {
	if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
	hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
	}
	*/
    return DefWindowProc (hwnd, message, wParam, lParam);
}

static void check_events (dx_instance_t * instance)
{
    MSG msg;
	
    while (PeekMessage (&msg, instance->window, 0, 0, PM_REMOVE)) {
		//    while (GetMessage (&msg, instance->window, 0, 0, PM_REMOVE)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
    }
}

static void wait_events (dx_instance_t * instance)
{
    MSG msg;
	
	//    while (PeekMessage (&msg, instance->window, 0, 0, PM_REMOVE)) {
    while (!key && !lMouseDown && GetMessage (&msg, NULL,0,0)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
    }
}

static int create_window (dx_instance_t * instance)
{
    RECT rect_window;
    WNDCLASSEX wc;

    wc.cbSize        = sizeof (WNDCLASSEX);
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = (WNDPROC) event_procedure;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle (NULL);
	hInst = wc.hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush (RGB (0, 0, 0));
//    wc.lpszMenuName  = (LPCSTR)IDC_GUI;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "libvo_dx";
    wc.hIconSm       = NULL;
    if (!RegisterClassEx (&wc)) {
	fprintf (stderr, "Can not register window class\n");
	return 1;
    }

    rect_window.top    = 10;
    rect_window.left   = 10;
    rect_window.right  = rect_window.left + instance->width;
    rect_window.bottom = rect_window.top + instance->height;
    AdjustWindowRect (&rect_window, WS_OVERLAPPEDWINDOW|WS_SIZEBOX, 0);

    instance->window = CreateWindow ("libvo_dx", instance->title,
				     WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
				     CW_USEDEFAULT, 0,
				     rect_window.right - rect_window.left,
				     rect_window.bottom - rect_window.top,
				     NULL, NULL, GetModuleHandle (NULL), NULL);
    if (instance->window == NULL) {
	fprintf (stderr, "Can not create window\n");
	return 1;
    }

    /* store a directx_instance pointer into the window local storage
     * (for later use in event_handler).
     * We need to use SetWindowLongPtr when it is available in mingw */
    SetWindowLong (instance->window, GWL_USERDATA, (LONG) instance);
	SetWindowPos(instance->window, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);

    ShowWindow (instance->window, SW_SHOW);

    return 0;
}

static LPDIRECTDRAWSURFACE2 alloc_surface (dx_instance_t * instance,
					   DDSURFACEDESC * ddsd)
{
    LPDIRECTDRAWSURFACE surface;
    LPDIRECTDRAWSURFACE2 surface2;

    if (DD_OK != IDirectDraw2_CreateSurface (instance->ddraw, ddsd,
					     &surface, NULL) ||
	DD_OK != IDirectDrawSurface_QueryInterface (surface,
						    &IID_IDirectDrawSurface2,
						    (LPVOID *) &surface2)) {
	fprintf (stderr, "Can not create directDraw frame surface\n");
	return NULL;
    }
    IDirectDrawSurface_Release (surface);

    return surface2;
}

static int dx_init (dx_instance_t *instance)
{
    HRESULT (WINAPI * OurDirectDrawCreate) (GUID *, LPDIRECTDRAW *,
					    IUnknown *);
    LPDIRECTDRAW ddraw;
    DDSURFACEDESC ddsd;

    /* load direct draw DLL */
    instance->hddraw_dll = LoadLibrary ("DDRAW.DLL");
    if (instance->hddraw_dll == NULL) {
	fprintf (stderr, "Can not load DDRAW.DLL\n");
	return 1;
    }

    ddraw = NULL;
    OurDirectDrawCreate = (void *) GetProcAddress (instance->hddraw_dll,
						   "DirectDrawCreate");
    if (OurDirectDrawCreate == NULL ||
	DD_OK != OurDirectDrawCreate (NULL, &ddraw, NULL) ||
	DD_OK != IDirectDraw_QueryInterface (ddraw, &IID_IDirectDraw2,
					     (LPVOID *) &instance->ddraw) ||
	DD_OK != IDirectDraw_SetCooperativeLevel (instance->ddraw,
						  instance->window,
						  DDSCL_NORMAL)) {
	fprintf (stderr, "Can not initialize directDraw interface\n");
	return 1;
    }
    IDirectDraw_Release (ddraw);

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    instance->display = alloc_surface (instance, &ddsd);
    if (instance->display == NULL) {
	fprintf (stderr, "Can not create directDraw display surface\n");
	return 1;
    }

    if (DD_OK != IDirectDraw2_CreateClipper (instance->ddraw, 0,
					     &instance->clipper, NULL) ||
	DD_OK != IDirectDrawClipper_SetHWnd (instance->clipper, 0,
					     instance->window) ||
	DD_OK != IDirectDrawSurface_SetClipper (instance->display,
						instance->clipper)) {
	fprintf (stderr, "Can not initialize directDraw clipper\n");
	return 1;
    }

    return 0;
}

static int common_setup (dx_instance_t * instance, int width, int height)
{
    instance->width = width;
    instance->height = height;
    instance->index = 0;

    if (create_window (instance) || dx_init (instance))
	return 1;
    return 0;
}

static int dxrgb_setup (vo_instance_t * _instance, unsigned int width,
			unsigned int height, unsigned int chroma_width,
			unsigned int chroma_height, vo_setup_result_t * result)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    HDC hdc;
    int bpp;

    if (common_setup (instance, width, height))
	return 1;

    hdc = GetDC (NULL);
    bpp = GetDeviceCaps (hdc, BITSPIXEL);
    ReleaseDC (NULL, hdc);

	memset(&birgb, 0, sizeof(BITMAPINFOHEADER));
	birgb.biSize = sizeof(BITMAPINFOHEADER);
	birgb.biWidth = width;
	birgb.biHeight = height;
	birgb.biPlanes = 1;
	birgb.biBitCount = 24;
	birgb.biCompression = BI_RGB;
	birgb.biSizeImage = width * height * 3;



//    result->convert = mpeg2convert_rgb (MPEG2CONVERT_RGB, bpp);
    return 0;
}

static int dx_setup (vo_instance_t * _instance, unsigned int width,
		     unsigned int height, unsigned int chroma_width,
		     unsigned int chroma_height, vo_setup_result_t * result)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 surface;
    DDSURFACEDESC ddsd;

    if (common_setup (instance, width, height))
	return 1;

    instance->overlay = alloc_overlay (instance);
    if (!instance->overlay)
	return 1;
    update_overlay (instance);

    surface = instance->overlay;

    /* Get the back buffer */
    memset (&ddsd.ddsCaps, 0, sizeof (DDSCAPS));
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (DD_OK != IDirectDrawSurface2_GetAttachedSurface (instance->overlay,
							 &ddsd.ddsCaps,
							 &surface))
	surface = instance->overlay;

    instance->yuv[0] = surface_addr (surface, &instance->stride);
    instance->yuv[2] = instance->yuv[0] + instance->stride * instance->height;
    instance->yuv[1] =
	instance->yuv[2] + (instance->stride * instance->height >> 2);

//    result->convert = NULL;
    return 0;
}


static LPDIRECTDRAWSURFACE2 alloc_frame (dx_instance_t * instance)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE2 surface;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwHeight = instance->height;
    ddsd.dwWidth = instance->width;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    surface = alloc_surface (instance, &ddsd);
    if (surface == NULL)
	fprintf (stderr, "Can not create directDraw frame surface\n");
    return surface;
}

static void * surface_addr (LPDIRECTDRAWSURFACE2 surface, int * stride)
{
    DDSURFACEDESC ddsd;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    IDirectDrawSurface2_Lock (surface, NULL, &ddsd,
			      DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    IDirectDrawSurface2_Unlock (surface, NULL);
    *stride = ddsd.lPitch;
    return ddsd.lpSurface;
}

static void dx_setup_fbuf (vo_instance_t * _instance,
			   uint8_t ** buf, void ** id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    int stride;

    *id = instance->frame[instance->index++] = alloc_frame (instance);
    buf[0] = surface_addr (*id, &stride);
    buf[1] = NULL;	buf[2] = NULL;
}

static void dxrgb_draw_frame (vo_instance_t * _instance,
			      uint8_t * const * buf, void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 surface = (LPDIRECTDRAWSURFACE2) id;
    DDBLTFX ddbltfx;

    check_events (instance);

	hdc = GetDC(instance->window);
	SetDIBitsToDevice(hdc, 0, 0, instance->width, instance->height,
		0, 0, 0, instance->height, *buf, (LPBITMAPINFO)lpbirgb, DIB_RGB_COLORS);

	return;

    memset (&ddbltfx, 0, sizeof (DDBLTFX));
    ddbltfx.dwSize = sizeof (DDBLTFX);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
    if (DDERR_SURFACELOST ==
	IDirectDrawSurface2_Blt (instance->display, &instance->window_coords,
				 surface, NULL, DDBLT_WAIT, &ddbltfx)) {
	/* restore surface and try again */
	IDirectDrawSurface2_Restore (instance->display);
	IDirectDrawSurface2_Blt (instance->display, &instance->window_coords,
				 surface, NULL, DDBLT_WAIT, &ddbltfx);
    }
}

static vo_instance_t * common_open (int setup (vo_instance_t *,
					       unsigned int, unsigned int,
					       unsigned int, unsigned int,
					       vo_setup_result_t *),
				    void setup_fbuf (vo_instance_t *,
						     uint8_t **, void **),
				    void draw (vo_instance_t *,
					       uint8_t * const *, void * id))
{
    dx_instance_t * instance;

    instance = malloc (sizeof (dx_instance_t));
    if (instance == NULL)
	return NULL;

    memset (instance, 0, sizeof (dx_instance_t));
//    instance->vo.setup = setup;
//    instance->vo.setup_fbuf = setup_fbuf;
//    instance->vo.set_fbuf = NULL;
//    instance->vo.start_fbuf = NULL;
//    instance->vo.draw = draw;
//    instance->vo.discard = NULL;
//    instance->vo.close = NULL; //dx_close;

    return (vo_instance_t *) instance;
}

vo_instance_t * vo_dxrgb_open (void)
{
    return common_open (dxrgb_setup, dx_setup_fbuf, dxrgb_draw_frame);
}

vo_instance_t * vo_dx_open (void)
{
    return common_open (dx_setup, NULL, dx_draw_frame);
}

static LPDIRECTDRAWSURFACE2 alloc_overlay (dx_instance_t * instance)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE2 surface;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwHeight = instance->height;
    ddsd.dwWidth = instance->width;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = FOURCC_YV12;
    ddsd.dwFlags |= DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
#if USE_OVERLAY_TRIPLE_BUFFERING
    ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
#endif
    ddsd.dwBackBufferCount = 2;

    surface = alloc_surface (instance, &ddsd);
    if (surface == NULL)
	fprintf (stderr, "Can not create directDraw frame surface\n");
    return surface;
}

static void copy_yuv_picture (dx_instance_t * instance,
			      uint8_t * const * buf, void * id)
{
    uint8_t * dest[3];
    int width, i;

    dest[0] = instance->yuv[0];
    dest[1] = instance->yuv[1];
    dest[2] = instance->yuv[2];

    width = instance->width;
    for (i = 0; i < instance->height >> 1; i++) {
	memcpy (dest[0], buf[0] + 2 * i * width, width);
	dest[0] += instance->stride;
	memcpy (dest[0], buf[0] + (2 * i + 1) * width, width);
	dest[0] += instance->stride;
	memcpy (dest[1], buf[1] + i * (width >> 1), width >> 1);
	dest[1] += instance->stride >> 1;
	memcpy (dest[2], buf[2] + i * (width >> 1), width >> 1);
	dest[2] += instance->stride >> 1;
    }
}

static void dx_draw_frame (vo_instance_t * _instance,
			   uint8_t * const * buf, void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;

    check_events (instance);

    copy_yuv_picture (instance, buf, id);

	if (instance->overlay) {
		if (DDERR_SURFACELOST ==
			IDirectDrawSurface2_Flip (instance->overlay, NULL, DDFLIP_WAIT)) {
			/* restore surfaces and try again */
			IDirectDrawSurface2_Restore (instance->display);
			IDirectDrawSurface2_Restore (instance->overlay);
			IDirectDrawSurface2_Flip (instance->overlay, NULL, DDFLIP_WAIT);
		}
	}
}



static void dx_close (vo_instance_t * _instance)
{
    dx_instance_t * instance;
    int i;

    instance = (dx_instance_t *) _instance;

    if (instance->overlay) {
	IDirectDrawSurface2_Release (instance->overlay);
	instance->overlay = NULL;
    } else
	for (i = 0; i < 3; i++) {
//	    if (instance->frame[i].p_surface != NULL)
//		IDirectDrawSurface2_Release (instance->frame[i].p_surface);
//	    instance->frame[i].p_surface = NULL;
	}

    if (instance->clipper != NULL)
	IDirectDrawClipper_Release (instance->clipper);

    if (instance->display != NULL)
	IDirectDrawSurface2_Release (instance->display);

    if (instance->ddraw != NULL)
	IDirectDraw2_Release (instance->ddraw);

    if (instance->hddraw_dll != NULL)
	FreeLibrary (instance->hddraw_dll);

    if (instance->window != NULL)
	DestroyWindow (instance->window);
}

#endif


//#undef RGB

#define MAXHEIGHT	1200
#define MAXWIDTH	2000

unsigned char buf0[MAXWIDTH*MAXHEIGHT*3];
unsigned char buf1[MAXWIDTH*MAXHEIGHT];
unsigned char buf2[MAXWIDTH*MAXHEIGHT];

unsigned char *(buffer[3]) = {buf0,buf1,buf2};

dx_instance_t * instance;
vo_setup_result_t result;

void vo_init(int width, int height, char *title) {
	memset(buf1,128,width*height);
	memset(buf2,128,width*height);

#ifdef RGB
	instance = vo_dxrgb_open();
	hWind = instance;
	strcpy(instance->title, title);
	dxrgb_setup( instance, width, height, width, height, &result);
//	dx_setup_fbuf ( instance, buffer, &result);
#else
	instance = vo_dx_open();
	strcpy(instance->title, title);
	dx_setup( instance, width, height, width, height, &result);
#endif

}

void vo_draw(char * buf)
{
	int id;
#ifdef RGB
//	dx_setup_fbuf ( instance, buffer, &result);
	memcpy(buffer[0], buf, instance->width*instance->height*3);
	dxrgb_draw_frame( instance,buffer , &id);
#else
	buffer[0] = buf;
	dx_draw_frame( instance, buffer , &id);
#endif
}


void vo_refresh()
{
	if (instance)
		check_events (instance);
}

void vo_wait()
{
	if (instance)
		wait_events (instance);
}



void vo_close()
{
	if (instance)
	    dx_close(instance);
}

int firstTime = 1;

void ShowDetails(char *t)
{
	int l;
	l = strlen(t);
	TextOut(hdc, 0, 30, t, l);
	if (firstTime) {
		TextOut(hdc, 0, 30+16, "Press F1 for help", 17);
		firstTime = 0;
	}
	hdc = BeginPaint(instance->window, &ps);
		EndPaint(instance->window, &ps);
		ReleaseDC(instance->window, hdc);

}

void ShowHelp(char **ta)
{
	char *t;
	int l;
	int i = 0;
	while (t = *ta) {
		l = strlen(t);
		TextOut(hdc, 0, 30+16*i++, t, l);
		ta++;
	}
		hdc = BeginPaint(instance->window, &ps);
		EndPaint(instance->window, &ps);
		ReleaseDC(instance->window, hdc);
}



#ifdef TEST

main()
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

	vo_init(MAXWIDTH,MAXHEIGHT, "test");
	
//	memset(buffer,0,sizeof(buffer));

	for (r = 0 ; r < 255; r++) {
		for (g = 0; g < 255; g++) {
			for (b=0; b<255;b++) {
#ifdef RGB
				buffer[0][2*g*MAXWIDTH+2*b] = r;
				buffer[0][2*g*MAXWIDTH+2*b+1] = g;
				buffer[0][(2*g+1)*MAXWIDTH+2*b] = b;
				buffer[0][(2*g+1)*MAXWIDTH+2*b+1] = r;

#else
				buffer[0][2*g*MAXWIDTH+2*b] = r;
				buffer[0][2*g*MAXWIDTH+2*b+1] = r;
				buffer[0][(2*g+1)*MAXWIDTH+2*b] = r;
				buffer[0][(2*g+1)*MAXWIDTH+2*b+1] = r;
				buffer[1][g*400+b] = 128;
				buffer[2][g*400+b] = 128;
#endif
			}
		}
		vo_draw(buffer[0]);
	}
}

#endif


#else
/* these need a home, even though this file is mostly not used */
int xPos,yPos,lMouseDown;
int key;
char osname[1024];
#endif
