/*

	View.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#include "View.h"
#include "Application.h"

#include <Window.h>
#include <ScrollBar.h>

#include "Messages.h"

// Universal BBitmap accessors. C++ inlines.
// Generic, but slower than the macros, because the pointer p has to be calculated for each pixel...
inline void get_pixel (BBitmap * B, int x, int y, uchar & r, uchar & g, uchar & b)
{
	uchar *p = (uchar*) B->Bits() + x * 4 + y * B->BytesPerRow ();
	b = *p;
	g = *(p + 1);
	r = *(p + 2);
}

inline void set_pixel (BBitmap * B, int x, int y, uchar r, uchar g, uchar b)
{
	uchar *p = (uchar*) B->Bits() + x * 4 + y * B->BytesPerRow ();
	*p = b;
	*(p + 1) = g;
	*(p + 2) = r;
}

View::View(BRect rect) : BView(rect, "View", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED)
{
	needRefresh = false;
	needSaving = false;
	sourceimage	= NULL;
	outputimage = NULL;
	viewimage = NULL;
	while_processing = false;
}

void
View::Draw(BRect rect)
{
	if (viewimage)
		DrawBitmap (viewimage, rect, rect);
}

void
View::FrameResized(float width, float height)
{
	FixupScrollbars();
}

void
View::SetSourceImage(BBitmap * source, const char * filename)
{
	if (sourceimage)
		delete sourceimage;
	sourceimage = source;
	sourcefilename = filename;
	FixupScrollbars();
	ViewSource();
	if (sourceimage)
		SetViewColor(B_TRANSPARENT_COLOR);
	else
		SetViewColor(0,0,0);
	if (outputimage)
		delete outputimage;
	outputimage = NULL;
	outputfilename = "";
}

void
View::SetOutputImage(BBitmap * output, const char * filename)
{
	if (outputimage)
		delete outputimage;
	outputimage = output;
	outputfilename = filename;
	if (outputimage)
		ViewOutput();
	else
		ViewSource();
}

void
View::BlurImage (ulong which)
{
	float	matrix1[] = { 6., 1., 1., 0.5 };
	int		radius1   = 1;
	float	matrix2[] = { 1., 1., 1., 0.5 };
	int		radius2   = 1;
	float	matrix3[] = { 8., 2., 1., 2., 2., 1., 1., 1., 0.5 };
	int		radius3   = 2;

	float * matrix = matrix1;
	int		radius = radius1;
	switch (which) {
		case menu_view_blur1: matrix = matrix1; radius = radius1; break;
		case menu_view_blur2: matrix = matrix2; radius = radius2; break;
		case menu_view_blur3: matrix = matrix3; radius = radius3; break;
	}

	BRect	f = viewimage->Bounds ();
	int	xmin = int (f.left) + radius;
	int xmax = int (f.right) - radius;
	int	ymin = int (f.top) + radius;
	int ymax = int (f.bottom) - radius;
	BBitmap * result = new BBitmap (f, B_RGB32);
	memcpy (result->Bits (), viewimage->Bits (), viewimage->BitsLength ());
	for (int y = ymin; y < ymax; y++) {
		ACCESS_BITMAP(d, result, 0, y);
		for (int x = xmin; x < xmax; x++) {
			float	rr, rg, rb;
			rr = rg = rb = 0.;
			float	tw = 0.;
			for (int rx = -radius; rx <= radius; rx++)
				for (int ry = -radius; ry <= radius; ry++) {
					int i = radius * (ry < 0 ? -ry : ry) + (rx < 0 ? -rx : rx);
					float w = matrix[i];
					tw += w;
					uchar	r, g, b;
					get_pixel (viewimage, x + rx, y + ry, r, g, b);
					rr += w * r;
					rg += w * g;
					rb += w * b;
				}
			uchar	r, g, b;
			r = int (rr / tw);
			g = int (rg / tw);
			b = int (rb / tw);
			SET_PIXEL(d, x, r, g, b);
		}
	}
	if (viewimage == sourceimage) {
		delete sourceimage;
		sourceimage = result;
		ViewSource();
	} else {
		delete outputimage;
		outputimage = result;
		ViewOutput();
		needSaving = true;
	}
}

void
View::Pulse()
{
	if (needRefresh) {
		needRefresh = false;
		if (viewimage)
			DrawBitmap (viewimage);
	}
}

void
View::MouseDown(BPoint where)
{
	if (viewimage == outputimage && while_processing) {
		BMessage m (fire_tile);
		m.AddPoint ("where", where);
		be_app->PostMessage (&m);
	}
}

void
View::ViewSource()
{
	viewimage = sourceimage;
	Invalidate ();
}

void
View::ViewOutput()
{
	viewimage = outputimage;
	Invalidate ();
}

void
View::GetMaxSize(float * maxx, float * maxy)
{
	if (!sourceimage) {
		*maxx = 200;
		*maxy = 200;
	} else {
		BRect rect = sourceimage->Bounds();
		*maxx = rect.right;
		*maxy = rect.bottom;
	}
}

void
View::FixupScrollbars()
{
	BRect bounds;
	BScrollBar * sb;

	bounds = Bounds();
	float myPixelWidth = bounds.Width();
	float myPixelHeight = bounds.Height();
	float maxWidth = 1;
	float maxHeight = 1;

	if (sourceimage != NULL) {
		// get max size of image
		GetMaxSize(&maxWidth, &maxHeight);
	}
		
	float propW = myPixelWidth/maxWidth;
	float propH = myPixelHeight/maxHeight;
	
	float rangeW = maxWidth - myPixelWidth;
	float rangeH = maxHeight - myPixelHeight;
	if (rangeW < 0) rangeW = 0;
	if (rangeH < 0) rangeH = 0;

	if ((sb=ScrollBar(B_HORIZONTAL))!=NULL) {
		sb->SetProportion(propW);
		sb->SetRange(0,rangeW);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(myPixelWidth / 8.0, myPixelWidth / 2.0);
	} 

	if ((sb=ScrollBar(B_VERTICAL))!=NULL) {
		sb->SetProportion(propH);
		sb->SetRange(0,rangeH);
		// Steps are 1/8 visible window for small steps
		//   and 1/2 visible window for large steps
		sb->SetSteps(myPixelHeight / 8.0, myPixelHeight / 2.0);
	}
}