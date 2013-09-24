/*

	Application.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Application.h>

#include "VideoConsumer.h"

class Window;
class BMediaRoster;
class VideoConsumer;
class BDiscreteParameter;

class Application : public BApplication
{
public:
	Application();
	
	virtual void	MessageReceived(BMessage * msg);
	virtual void	RefsReceived(BMessage *message);
	virtual bool 	QuitRequested();

private:
	void			StartProcessing(bool resume);
	
	void			MakeTile();
	float			Compare(int bx, int by);
	void			Copy(int bx, int by);
	
	void			Process();
	friend			long process_thread (void *arg);
	void			Retire ();
	void			SaveImage();
	
	status_t		SetupNodes();
	void			TearDownNodes();
	void			CheckMonitoring (bool stop = false);
	
	//Stuff for SetupNodes
	BMediaRoster *	fMediaRoster;
	media_node		fTimeSourceNode;
	media_node		fProducerNode;
	media_output	fProducerOut;
	VideoConsumer *	fVideoConsumer;
	media_input		fConsumerIn;
	port_id			fPort;
	BDiscreteParameter * fTuner;
		
	//Other stuff
	Window *		window;

	thread_id		processorID;

	BBitmap *		tile;
	int				left;
	int				xtiles;
	int				ytiles;

	float			error[1000][1000];
	
	bool			doprocess;
};

// Shortcut methods to access bitmap while letting the compiler maximum visibility for optimization.
// Two parts: initialization done by the ACCESS_BITMAP macro, access by the SET_PIXEL/GET_PIXEL macros.
// For all of these calls, the following is assumed for the named parameters:
// A: is the name of a variable used to identify which bitmap you are accessing. Just give a name which
//    can be used in that context. Warning: It also uses the name "AA", that is for instance,
//    if you give "myname", then both "myname" & "mynamemyname" are defined and should not be used!
// b: is a BBitmap * to the bitmap you want to access.
// x, y for ACCESS_BITMAP: The first pixel you want to access in the y raw.
// x for GET_PIXEL/SET_PIXEL: The pixel to access in the raw declared in ACCESS_BITMAP, starting after
//                          x value given in ACCESS_BITMAP.
// Beware that x & y must be integer! Force the type if necessary!
// Look at the code for examples!
#define ACCESS_BITMAP(A, b, x, y) uchar *A = (uchar*) (b)->Bits() + (x) * 4 + (y) * ((b)->BytesPerRow ())
#define SET_PIXEL(A, x, r, g, b) uchar* A##A = A + 4 * (x); *A##A = b; *(A##A + 1) = g; * (A##A + 2) = r
#define GET_PIXEL(A, x, r, g, b) uchar* A##A = A + 4 * (x); b = *A##A; g = *(A##A + 1); r = * (A##A + 2)

#endif // _APPLICATION_H_
