/*

	View.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#ifndef _VIEW_H_
#define _VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <String.h>

class View : public BView
{
public:
					View(BRect rect);
	virtual void	Draw(BRect rect);
	virtual void	FrameResized(float width, float height);
	virtual	void	Pulse ();
	virtual	void	MouseDown(BPoint where);

	void			SetSourceImage(BBitmap * source = NULL, const char * filename = "");
	void			SetOutputImage(BBitmap * output = NULL, const char * filename = "");
	void			GetMaxSize(float * maxx, float * maxy);
	void			ViewSource();
	void			ViewOutput();
	
	void			BlurImage (ulong which);
	
	bool		needRefresh;
	bool		needSaving;
	bool		while_processing;
	
	BString		sourcefilename;
	BBitmap *	sourceimage;

	BString		outputfilename;
	BBitmap *	outputimage;
	BBitmap * 	viewimage;

private:
	void		FixupScrollbars();
};

#endif // _VIEW_H_