
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include "icon_bitmap.h"

#define BITMAPDEPTH 1
#define TOO_SMALL 0
#define BIG_ENOUGH 1
/* These are used as arguments to nearly every Xlib routine, so it
 * saves routine arguments to declare them global; if there were
 * additional source files, they would be declared extern there */
Display *display;
int screen_num;
/* progname is the string by which this program was invoked; this
 * is global because it is needed in most application functions */
static char *progname;

void load_font(XFontStruct *font_info);
void getGC(Window win, GC *gc, XFontStruct *font_info);
void place_text(Window win, GC gc, XFontStruct *font_info, unsigned int win_width, unsigned int win_height);
void place_graphics(Window win, GC gc, unsigned int window_width, unsigned int window_height);
void TooSmall(Window win, GC gc, XFontStruct *font_info);

void main(int argc, char **argv)
{
	Window win;
	unsigned int width, height;
	int x, y;
	unsigned int border_width = 4;
	unsigned int display_width, display_height;
	char *window_name = "Basic Window Program";
	char *icon_name = "basicwin";
	Pixmap icon_pixmap;
	XSizeHints *size_hints;
	XIconSize *size_list;
	XWMHints *wm_hints;
	XClassHint *class_hints;
	XTextProperty windowName, iconName;
	int count;
	XEvent report;
	GC gc;
	XFontStruct font_info;
	char *display_name = NULL;
	int window_size = 0;

	progname = argv[0];
	if (!(size_hints = XAllocSizeHints()))
	{
		fprintf(stderr, "%s: failure allocating memory", progname);
		exit(0);
	}
	if (!(wm_hints = XAllocWMHints()))
	{
		fprintf(stderr, "%s: failure allocating memory", progname);
		exit(0);
	}
	if (!(class_hints = XAllocClassHint()))
	{
		fprintf(stderr, "%s: failure allocating memory", progname);
		exit(0);
	}
	/* Connect to X server */
	if ((display = XOpenDisplay(display_name)) == NULL)
	{
		(void)fprintf(stderr, "%s: cannot connect to X server %s\n",
									progname, XDisplayName(display_name));
		exit(1);
	}
	/* Get screen size from display structure macro */
	screen_num = DefaultScreen(display);
	display_width = DisplayWidth(display, screen_num);
	display_height = DisplayHeight(display, screen_num);
	/* Note that in a real application, x and y would default
	 * to 0 but would be settable from the command line or
	 * resource database */
	x = y = 0;
	/* Size window with enough room for text */
	width = display_width / 3, height = display_height / 4;
	/* Create opaque window */
	win = XCreateSimpleWindow(display, RootWindow(display, screen_num),
														x, y, width, height,
														border_width, BlackPixel(display, screen_num), WhitePixel(display, screen_num));

	/* Get available icon sizes from window manager */
	// if (XGetIconSizes(display, RootWindow(display, screen_num), &size_list, &count) == 0)
	//	(void)fprintf(stderr, "%s: Window manager didn’t set icon sizes - using default.\n", progname);
	// else
	//{
	//	;
	//	/* A real application would search through size_list
	//	 * here to find an acceptable icon size and then
	//	 * create a pixmap of that size; this requires that
	//	 * the application have data for several sizes of icons */
	// }

	/* Create pixmap of depth 1 (bitmap) for icon */
	icon_pixmap = XCreateBitmapFromData(display, win, icon_bitmap_bits, icon_bitmap_width, icon_bitmap_height);
	/* Set size hints for window manager; the window manager
	 * may override these settings */
	/* Note that in a real application, if size or position
	 * were set by the user, the flags would be USPosition
	 * and USSize and these would override the window manager’s
	 * preferences for this window */
	/* x, y, width, and height hints are now taken from
	 * the actual settings of the window when mapped; note
	 * that PPosition and PSize must be specified anyway */
	size_hints->flags = PPosition | PSize | PMinSize;
	size_hints->min_width = 300;
	size_hints->min_height = 200;
	/* These calls store window_name and icon_name into
	 * XTextProperty structures and set their other fields
	 * properly */
	if (XStringListToTextProperty(&window_name, 1, &windowName) == 0)
	{
		(void)fprintf(stderr, "%s: structure allocation for windowName failed.\n", progname);
		exit(1);
	}
	if (XStringListToTextProperty(&icon_name, 1, &iconName) == 0)
	{
		(void)fprintf(stderr, "%s: structure allocation for iconName failed.\n", progname);
		exit(1);
	}
	wm_hints->initial_state = NormalState;
	wm_hints->input = True;
	wm_hints->icon_pixmap = icon_pixmap;
	wm_hints->flags = StateHint | IconPixmapHint | InputHint;
	class_hints->res_name = progname;
	class_hints->res_class = "Basicwin";
	XSetWMProperties(display, win, &windowName, &iconName,
									 argv, argc, size_hints, wm_hints,
									 class_hints);
	/* Select event types wanted */

	XSelectInput(display, win, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);
	load_font(&font_info);
	/* Create GC for text and drawing */
	getGC(win, &gc, &font_info);
	/* Display window */
	XMapWindow(display, win);
	/* Get events, use first to display text and graphics */
	XFlush(display);

	while (1)
	{
		XNextEvent(display, &report);
		switch (report.type)
		{
		case Expose:
			/* Unless this is the last contiguous expose,
			 * don’t draw the window */
			if (report.xexpose.count != 0)
				break;
			/* If window too small to use */
			if (window_size == TOO_SMALL)
				TooSmall(win, gc, &font_info);
			else
			{
				/* Place text in window */
				place_text(win, gc, &font_info, width, height);
				/* Place graphics in window */
				place_graphics(win, gc, width, height);
			}
			break;
		case ConfigureNotify:
			/* Window has been resized; change width
			 * and height to send to place_text and
			 * place_graphics in next Expose */
			width = report.xconfigure.width;
			height = report.xconfigure.height;
			if ((width < size_hints->min_width) ||
					(height < size_hints->min_height))
				window_size = TOO_SMALL;
			else
				window_size = BIG_ENOUGH;
			break;
		case ButtonPress:
		/* Trickle down into KeyPress (no break) */
		case KeyPress:
			XUnloadFont(display, font_info.fid);
			XFreeGC(display, gc);
			XCloseDisplay(display);
			exit(1);
		default:
			/* All events selected by StructureNotifyMask
			 * except ConfigureNotify are thrown away here,
			 * since nothing is done with them */
			break;
		} /* End switch */
	} /* End while */
}

void getGC(Window win, GC *gc, XFontStruct *font_info)
{
	unsigned long valuemask = 0; /* Ignore XGCvalues and
																* use defaults */
	XGCValues values;
	unsigned int line_width = 6;
	int line_style = LineOnOffDash;
	int cap_style = CapRound;
	int join_style = JoinRound;
	int dash_offset = 0;
	static char dash_list[] = {12, 24};
	int list_length = 2;
	/* Create default Graphics Context */
	*gc = XCreateGC(display, win, valuemask, &values);
	/* Specify font */
	XSetFont(display, *gc, font_info->fid);
	/* Specify black foreground since default window background
	 * is white and default foreground is undefined */
	XSetForeground(display, *gc, BlackPixel(display, screen_num));
	/* Set line attributes */
	XSetLineAttributes(display, *gc, line_width, line_style,
										 cap_style, join_style);
	/* Set dashes */
	XSetDashes(display, *gc, dash_offset, dash_list, list_length);
}

void load_font(XFontStruct *font_info)
{
	char *fontname = "9x15";
	/* Load font and get font information structure */
	XFontStruct *p = XLoadQueryFont(display, fontname);

	if (p == NULL)
	{
		fprintf(stderr, "%s: Cannot open font\n", progname);
		exit(1);
	}

	memcpy(font_info, p, sizeof(XFontStruct));
}

void place_text(Window win, GC gc, XFontStruct *font_info, unsigned int win_width, unsigned int win_height)
{
	char *string1 = "Hi! I'm a window, who are you?";
	char *string2 = "To terminate program; Press any key";
	char *string3 = "or button while in this window.";
	char *string4 = "Screen Dimensions:";
	int len1, len2, len3, len4;
	int width1, width2, width3;
	char cd_height[50], cd_width[50], cd_depth[50];
	int font_height;
	int initial_y_offset, x_offset;
	/* Need length for both XTextWidth and XDrawString */
	len1 = strlen(string1);
	len2 = strlen(string2);
	len3 = strlen(string3);
	/* Get string widths for centering */
	width1 = XTextWidth(font_info, string1, len1);
	width2 = XTextWidth(font_info, string2, len2);
	width3 = XTextWidth(font_info, string3, len3);
	font_height = font_info->ascent + font_info->descent;
	/* Output text, centered on each line */
	XDrawString(display, win, gc, (win_width - width1) / 2, font_height, string1, len1);
	XDrawString(display, win, gc, (win_width - width2) / 2, (int)(win_height - (2 * font_height)), string2, len2);
	XDrawString(display, win, gc, (win_width - width3) / 2, (int)(win_height - font_height), string3, len3);
	/* Copy numbers into string variables */
	(void)sprintf(cd_height, " Height - %d pixels", DisplayHeight(display, screen_num));
	(void)sprintf(cd_width, " Width - %d pixels", DisplayWidth(display, screen_num));
	(void)sprintf(cd_depth, " Depth - %d plane(s)", DefaultDepth(display, screen_num));
	/* Reuse these for same purpose */
	len4 = strlen(string4);
	len1 = strlen(cd_height);
	len2 = strlen(cd_width);
	len3 = strlen(cd_depth);
	/* To center strings vertically, we place the first string
	 * so that the top of it is two font_heights above the center
	 * of the window; since the baseline of the string is what
	 * we need to locate for XDrawString and the baseline is
	 * one font_info -> ascent below the top of the character,
	 * the final offset of the origin up from the center of
	 * the window is one font_height + one descent */
	initial_y_offset = win_height / 2 - font_height - font_info->descent;
	x_offset = (int)win_width / 4;
	XDrawString(display, win, gc, x_offset, (int)initial_y_offset, string4, len4);
	XDrawString(display, win, gc, x_offset, (int)initial_y_offset + font_height, cd_height, len1);
	XDrawString(display, win, gc, x_offset, (int)initial_y_offset + 2 * font_height, cd_width, len2);
	XDrawString(display, win, gc, x_offset, (int)initial_y_offset + 3 * font_height, cd_depth, len3);
}

void place_graphics(Window win, GC gc, unsigned int window_width, unsigned int window_height)
{
	int x, y;
	int width, height;
	height = window_height / 2;
	width = 3 * window_width / 4;
	x = window_width / 2 - width / 2; /* Center */
	y = window_height / 2 - height / 2;
	XDrawRectangle(display, win, gc, x, y, width, height);
}

void TooSmall(Window win, GC gc, XFontStruct *font_info)
{
	char *string1 = "Too Small";
	int y_offset, x_offset;
	y_offset = font_info->ascent + 2;
	x_offset = 2;
	/* Output text, centered on each line */
	XDrawString(display, win, gc, x_offset, y_offset, string1, strlen(string1));
}