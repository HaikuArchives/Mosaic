/*

	Application.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#include "Application.h"
#include "Window.h"
#include "View.h"
#include "Messages.h"

#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <ParameterWeb.h>
#include <scheduler.h>
#include <File.h>
#include <stdio.h>
#include <Alert.h>

// Code for Error value big enough
const float kInitError = 1E20;

//	Definition: kTVx & kTVy
//		The pic size used by TV acquisition
// 		It is also the natural size of the monitor

//	Definition: kCrop
//		How many lines from the top & bottom to cut out
//		(to cut out black bands of movies and/or tv logos)

//	Definition: kTileX & kTileY
//		The size of extracted pic. It should be smaller
//		than half the pic size *after the bands cut* for good results.

//const int  kTVx = 80;
//const int  kTVy = 60;
//const int  kCrop = 7;

//const int  kTVx = 160;
//const int  kTVy = 120;
//const int  kCrop = 14;

const int kTVx = 320;
const int kTVy = 240;
const int  kCrop = 25;

//const int  kTVx = 640;
//const int  kTVy = 480;
//const int  kTileX = 40;
//const int  kTileY = 30;
// kCoef is just a short cut to help keep 4/3 proportions right...
const int  kCoef = 10;
const int  kTileX = (4*kCoef);
const int  kTileY = (3*kCoef);
//const int  kTileX = (32);
//const int  kTileY = (24);

void
Application::MakeTile()
{
	uchar	r, g, b;
	float	firsty = float (kCrop);
	float	lasty  = kTVy - firsty;
	float	stepy  = (lasty - firsty) / float (kTileY);
	float	height = lasty - firsty;
	float	firstx = (kTVx - height) / 2.0;
	float	lastx  = (kTVx + height) / 2.0;
	float	stepx  = (lastx - firstx) / float (kTileX);

//	Minimal sampling for the tile
//	float	tvy = firsty;
//	for (int y = 0; y < kTileY; y++, tvy += stepy) {
//		float	tvx = firstx;
//		ACCESS_BITMAP(s, fVideoConsumer->image, 0, int(tvy));
//		ACCESS_BITMAP(d, tile, 0, y);
//		for (int x = 0; x < kTileX; x++, tvx += stepx) {
//			GET_PIXEL(s, int(tvx), r, g, b);
//			SET_PIXEL(d, x, r, g, b);
//		}
//	}

// Two pixels to one aliasing. Note that each generated pixel uses distinct source pixels
// !!!!!! YOU MUST HAVE kCrop > 0 FOR THIS TO WORK !!!!!
// This provides a very slight horizontal oversampling of the source.
// Gives a slight aliasing while keeping edges
	float	tvy = firsty;
	for (int y = 0; y < kTileY; y++, tvy += stepy) {
		ACCESS_BITMAP(s, fVideoConsumer->image, 0, int(tvy));
		ACCESS_BITMAP(d, tile, 0, y);
		float	tvx = firstx - stepx / 4;
		for (int x = 0; x < kTileX; x++, tvx += stepx) {
			uchar	pr, pg, pb;
			GET_PIXEL(s, int(tvx), pr, pg, pb);
			int 	n = int (tvx + stepx / 2);
			{	// declares temp variable... Hide it from previous one!
				GET_PIXEL(s, n, r, g, b);
			}
			pr = (int (pr) + int (r)) / 2;
			pg = (int (pg) + int (g)) / 2;
			pb = (int (pb) + int (b)) / 2;
			SET_PIXEL(d, x, pr, pg, pb);
		}
	}
}

float
Application::Compare(int bx, int by){
	// mesure the difference between the current TV capture & one spot of the original pic.
	// 0 means equal. The value raises as the difference grows.
	// The result should be smaller than kInitError.
	// Appart from that, how this value is calculated is up to you: experiment!
	uchar tr, tg, tb;
	uchar sr, sg, sb;

	float result = 0;
	for (int y = 0; y < kTileY; y++) {
		ACCESS_BITMAP(t, tile, 0, y);
		ACCESS_BITMAP(s, window->view->sourceimage, bx * kTileX, by * kTileY + y);
		for (int x = 0; x < kTileX; x++) {
			GET_PIXEL(t, x, tr, tg, tb);
			GET_PIXEL(s, x, sr, sg, sb);
			float r = tr - sr;
			float g = tg - sg;
			float b = tb - sb;
			result += (r*r + g*g + b*b);
//			result += (fabsf (r) + fabsf (g) + fabsf (b));
		}
	}
	return result;
}

// The code below may be changed, of course, but interesting functions & parameters
// to play with are all above!

int
main()
{
	Application		app;
	app.Run();
}

Application::Application() : BApplication("application/x-vnd.Mosaic")
{
	processorID = -1;
	tile = new BBitmap (BRect (0, 0, kTileX - 1, kTileY - 1), B_RGB32);
	window = new Window ();
	fTuner = NULL;
}

void
Application::MessageReceived(BMessage * msg)
{
	switch (msg->what) {
	case stop_processing:
		Retire ();
		break;
	case save_result:
		Retire ();
		SaveImage ();
		break;
	case start_processing:
		StartProcessing(false);
		break;
	case start_processing_with_pic:
		StartProcessing(true);
		break;
	case fire_tile:
	{	// The user is unhappy with that tile: give it a bad score!
		BPoint	where;
		if (msg->FindPoint ("where", &where) == B_OK) {
			int	x, y;
			x = int (where.x) / kTileX;
			y = int (where.y) / kTileY;
			if (x >= 0 && x < xtiles && y >= 0 && y < ytiles)
				error[x][y] = kInitError;
		}
		break;
	}
	case app_close_monitoring:
		CheckMonitoring (true);
		break;
	case app_switch_monitoring:
		CheckMonitoring ();
		break;
	case menu_TV_channel: {
		int32		channel;
		const char*	name;
		if (fTuner && msg->FindString("name", &name) == B_OK && msg->FindInt32("channel", &channel) == B_OK) {
			int32	lastChannel = fTuner->CountItems() - 1;
			if (channel > lastChannel)
				channel = lastChannel;
			if (channel < 0)
				channel = 0;
			int32 current_channel;
			bigtime_t time;
			size_t size = sizeof (int32);
			if ((fTuner->GetValue(reinterpret_cast<void *>(&current_channel), &size, &time) == B_OK) && (size <= sizeof(int32))) {
				fVideoConsumer->SetChannelName (name);
				if (current_channel != channel)
					fTuner->SetValue(reinterpret_cast<void *>(&channel), sizeof (int32), BTimeSource::RealTime());
			}
			BMessage m (window_check_channel);
			m.AddInt32 ("channel", channel);
			window->PostMessage (&m);
		}
		break;
	}
	default:
		BApplication::MessageReceived(msg);
	}
}

void
Application::RefsReceived(BMessage *message)
{
	ulong		type;
	long		count;

	message->GetInfo("refs", &type, &count);
	if (type == B_REF_TYPE && count > 0) {
		message->what = B_REFS_RECEIVED;
		window->PostMessage (message);
	}
}

bool 
Application::QuitRequested()
{
	Retire ();
		
	if (window->view->needSaving) {
		BAlert * now_what = new BAlert("Um...", "Do you want to save the current result before quitting?",
			"Cancel", "Quit Anyway", "Save", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		now_what->SetShortcut(0, B_ESCAPE);
		int choice = now_what->Go();
		if (choice == 1) {
		} else if (choice == 2) {
			window->PostMessage (menu_file_save);
			return false;
		} else
			return false;
	}

	TearDownNodes();
	return true;
}

long process_thread (void *arg)
{
	Application * app = (Application*) arg;
	app->Process ();
	return B_OK;
}

void
Application::StartProcessing(bool resume)
{
	//Start video input stream
	SetupNodes();

	window->view->while_processing = true;
	doprocess = true;

	// Prepare the work variables: xtiles, ytiles, left, error
	xtiles = int (window->view->sourceimage->Bounds().Width() / kTileX);
	ytiles = int (window->view->sourceimage->Bounds().Height() / kTileY);
	if (resume) {
		left = 0;
		for (int y = 0; y < ytiles; y++){
			for (int x = 0; x < xtiles; x++){
				uchar r, g, b;
			
				int	blanc = 0;
				// Copy the part of target image into the tile to be able to use Compare...
				for (int yy = 0; yy < kTileY; yy++) {
					ACCESS_BITMAP(s, window->view->outputimage, x * kTileX, y * kTileY + yy);
					ACCESS_BITMAP(d, tile, 0, yy);
					for (int xx = 0; xx < kTileX; xx++) {
						GET_PIXEL(s, xx, r, g, b);
						SET_PIXEL(d, xx, r, g, b);
						if (r == 255 && g == 255 && b == 255)
							blanc ++;
					}
				}
				if (blanc == kTileX * kTileY) {
					// This tile is white: is it not yet initialized!
					left ++;
					error[x][y] = kInitError;
				} else
					error[x][y] = Compare (x, y);
			}
		}
	} else {
		for (int y = 0; y < ytiles; y++) {
			for (int x = 0; x < xtiles; x++) {
				error[x][y] = kInitError;
			}
		}
		left = xtiles * ytiles;
	}
	
	// Create an independent thread for the calculations
	processorID = spawn_thread(process_thread, "Processing thread", B_NORMAL_PRIORITY, this);
	resume_thread (processorID);
	window->PostMessage(menu_view_output);
}

void
Application::Process()
{
	while (doprocess)
	{
		MakeTile();
		
		int bestX = 0;
		int bestY = 0;
		float bestImprove = 0;
		float bestError = kInitError;
		
		for (int y = 0; y < ytiles; y++) {
			for (int x = 0; x < xtiles; x++) {
				if (left > 0) {
					if (error[x][y] == kInitError) {
						/////////////////////////////////////////////
						float thisError = Compare(x, y);
						if (thisError < bestError) {
							bestX = x;
							bestY = y;
							bestError = thisError;
						}
						/////////////////////////////////////////////					
					}
				}
				else
				{
					/////////////////////////////////////////////
					float thisError = Compare(x, y);
					float Improve = error[x][y] - thisError;
					if (Improve > bestImprove) {
						bestX = x;
						bestY = y;
						bestImprove = Improve;
						bestError = thisError;
					}
					/////////////////////////////////////////////	
				}	
			}
		}
	
		if (bestError < kInitError) {
			if (error[bestX][bestY] == kInitError)
				left--;
			Copy (bestX, bestY);
			error[bestX][bestY] = bestError;
			window->view->needRefresh = true;
			window->view->needSaving = true;
			// Limit that a similar image is used too many times
			if (left <= 0)	// initializing phase is accepted
				snooze (10000);
		}
	}
	window->view->while_processing = false;
	processorID = -1;
}

void
Application::Retire()
{
	doprocess = false;
	thread_info	infos;
	while (get_thread_info (processorID, &infos) == B_OK)
		snooze (20000);
}

void
Application::Copy(int bx, int by){
	uchar r, g, b;
	for (int y = 0; y < kTileY; y++) {
		ACCESS_BITMAP(s, tile, 0, y);
		ACCESS_BITMAP(d, window->view->outputimage, bx * kTileX, by * kTileY + y);
		for (int x = 0; x < kTileX; x++) {
			GET_PIXEL(s, x, r, g, b);
			SET_PIXEL(d, x, r, g, b);
		}
	}
}

void
Application::SaveImage()
{
	if (window->view->outputimage) {
		BTranslatorRoster * roster = BTranslatorRoster::Default();
		BBitmapStream stream(window->view->outputimage);
		BFile file(window->view->outputfilename.String(), B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
		if (file.InitCheck () == B_OK) {
			window->view->needSaving = false;
			uint32 type = B_PNG_FORMAT;
			roster->Translate(&stream, NULL, NULL, &file, type);
			stream.DetachBitmap(&window->view->outputimage);
		}
	}
}

static void
ErrorAlert(const char * message, status_t err)
{
	(new BAlert("", message, "Quit"))->Go();
	be_app->PostMessage(B_QUIT_REQUESTED);
}

status_t
Application::SetupNodes()
{
	if (fVideoConsumer)
		return B_OK;

	status_t status = B_OK;

	/* find the media roster */
	fMediaRoster = BMediaRoster::Roster(&status);
	if (status != B_OK) {
		ErrorAlert("Can't find the media roster", status);
		return status;
	}	
	/* find the time source */
	status = fMediaRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		ErrorAlert("Can't get a time source", status);
		return status;
	}
	/* find a video producer node */
	status = fMediaRoster->GetVideoInput(&fProducerNode);
	if (status != B_OK) {
		ErrorAlert("Can't find a video input!", status);
		return status;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer();
	if (!fVideoConsumer) {
		ErrorAlert("Can't create a video window", B_ERROR);
		return B_ERROR;
	}
	
	/* register the node */
	status = fMediaRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert("Can't register the video window", status);
		return status;
	}
	fPort = fVideoConsumer->ControlPort();
	
	/* find free producer output */
	int32 cnt = 0;
	status = fMediaRoster->GetFreeOutputsFor(fProducerNode, &fProducerOut, 1,  &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available video stream", status);
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fMediaRoster->GetFreeInputsFor(fVideoConsumer->Node(), &fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		status = B_RESOURCE_UNAVAILABLE;
		ErrorAlert("Can't find an available connection to the video window", status);
		return status;
	}

	/* Connect The Nodes!!! */

	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	media_raw_video_format vid_format = 
		{ 0, 0, 0, 239, B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB32, kTVx + 1, kTVy + 1, (kTVx + 1)*4, 0, 0}};
	format.u.raw_video = vid_format; 
	
	/* connect producer to consumer */
	status = fMediaRoster->Connect(fProducerOut.source, fConsumerIn.destination,
				&format, &fProducerOut, &fConsumerIn);
	if (status != B_OK) {
		ErrorAlert("Can't connect the video source to the video window", status);
		return status;
	}
	
	/* set time sources */
	status = fMediaRoster->SetTimeSourceFor(fProducerNode.node, fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video source", status);
		return status;
	}
	
	status = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video window", status);
		return status;
	}
	
	/* figure out what recording delay to use */
	bigtime_t latency = 0;
	status = fMediaRoster->GetLatencyFor(fProducerNode, &latency);
	status = fMediaRoster->SetProducerRunModeDelay(fProducerNode, latency);

	/* start the nodes */
	bigtime_t initLatency = 0;
	status = fMediaRoster->GetInitialLatencyFor(fProducerNode, &initLatency);
	if (status < B_OK) {
		ErrorAlert("error getting initial latency for fCaptureNode", status);	
	}
	initLatency += estimate_max_scheduling_latency();
	
	BTimeSource *timeSource = fMediaRoster->MakeTimeSourceFor(fProducerNode);
	bool running = timeSource->IsRunning();
	
	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running)
	{
		status = fMediaRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot start time source!", status);
			return status;
		}
		status = fMediaRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("cannot seek time source!", status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
	timeSource->Release();
	
	/* start the nodes */
	status = fMediaRoster->StartNode(fProducerNode, perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video source", status);
		return status;
	}
	status = fMediaRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video window", status);
		return status;
	}
	
	if (status == B_OK) {
		snooze (100000); // wait a little that images really arrive, or you'll have blank pics!
		BMediaRoster *roster = BMediaRoster::CurrentRoster ();
		BParameterWeb *web;
		if ((roster->GetParameterWebFor (fProducerNode, &web) == B_OK) && web)
		{
			for (int32 i = 0; i < web->CountParameters(); i++)
			{
				BParameter *parameter = web->ParameterAt(i);
				if (strcmp (parameter->Name (), "Channel:") == 0)
				{
					fTuner = dynamic_cast<BDiscreteParameter *> (parameter);
					int32 current_channel;
					bigtime_t time;
					size_t size = sizeof (int32);
					if ((fTuner->GetValue(reinterpret_cast<void *>(&current_channel), &size, &time) == B_OK) && (size <= sizeof(int32))) {
						BMessage m (window_check_channel);
						m.AddInt32 ("channel", current_channel);
						window->PostMessage (&m);
					}
					break;
				}
			}
		}
		CheckMonitoring ();
	}

	return status;
}

void
Application::TearDownNodes()
{
	CheckMonitoring (true);
	if (!fMediaRoster)
		return;
	
	if (fVideoConsumer)
	{
		fMediaRoster->StopNode(fVideoConsumer->Node(), 0, true);
		fMediaRoster->Disconnect(fProducerOut.node.node, fProducerOut.source,
								fConsumerIn.node.node, fConsumerIn.destination);
								
		if (fProducerNode != media_node::null) {
			fMediaRoster->ReleaseNode(fProducerNode);
			fProducerNode = media_node::null;
		}
		fMediaRoster->ReleaseNode(fVideoConsumer->Node());		
		fVideoConsumer = NULL;
	}
}

void
Application::CheckMonitoring (bool stop = false)
{
	if (stop)
		window->monitoring = false;
	if (fVideoConsumer)
		fVideoConsumer->Show (window->monitoring, BPoint (200, 200));
}
