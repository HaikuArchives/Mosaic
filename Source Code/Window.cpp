/*

	Window.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#include "Window.h"
#include "View.h"

#include <Application.h>
#include <ScrollView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <FilePanel.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <Alert.h>
#include <Path.h>
#include <fs_attr.h>
#include <Node.h>

#include "Messages.h"

#include <iostream.h>

Window::Window() : BWindow(BRect(100,100,299,299), "Mosaic", B_DOCUMENT_WINDOW_LOOK , B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
{
	monitoring = true;
	//Add user menus
	BMenuBar * menubar;
	menubar = new BMenuBar(BRect(0,0,Frame().Width(),15), "Menubar");

	//Add file menu
	BMenu * 	menu = new BMenu("File");
	BMenuItem * menuitem;
	
	menuitem_view_open = new BMenuItem("Open Source Image...", new BMessage(menu_file_opensource), 'O');
	menu->AddItem(menuitem_view_open);
	menuitem_view_reload_source = new BMenuItem("Reload Source Image", new BMessage(menu_file_reload_source));
	menu->AddItem(menuitem_view_reload_source);
	menuitem_view_continue = new BMenuItem("Select Result Image...", new BMessage(menu_file_continue), 'R');
	menu->AddItem(menuitem_view_continue);
	menuitem_view_reload_output = new BMenuItem("Reload Result Image", new BMessage(menu_file_reload_output));
	menu->AddItem(menuitem_view_reload_output);

	menu->AddSeparatorItem();
	menuitem_view_start = new BMenuItem("Start Processing", new BMessage(menu_file_work), 'P');
	menu->AddItem(menuitem_view_start);
	menuitem_view_stop = new BMenuItem("Stop Processing", new BMessage(stop_processing), 'K');
	menuitem_view_stop->SetTarget (be_app_messenger);
	menu->AddItem(menuitem_view_stop);
	
	menu->AddSeparatorItem();
	menuitem_view_save = new BMenuItem("Save Result", new BMessage(menu_file_save), 'S');
	menu->AddItem(menuitem_view_save);
	menuitem_view_save_as = new BMenuItem("Save Result As...", new BMessage(menu_file_save_as));
	menu->AddItem(menuitem_view_save_as);
	
	menu->AddSeparatorItem();
	menuitem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	menuitem->SetTarget(be_app); 	
	menu->AddItem(menuitem);
	
	menubar->AddItem(menu);

	//Add view menu
	menu = new BMenu("Image");
	menuitem_view_source = new BMenuItem("View Source", new BMessage(menu_view_source), 'I');
	menu->AddItem(menuitem_view_source);
	menuitem_view_source->SetMarked(true);
	
	menuitem_view_output = new BMenuItem("View Result", new BMessage(menu_view_output), 'U');
	menu->AddItem(menuitem_view_output);
	
	menu->AddSeparatorItem();
	menuitem_view_blur_little = new BMenuItem("Blur this Image just a Little", new BMessage(menu_view_blur1), '1');
	menu->AddItem(menuitem_view_blur_little);
	menuitem_view_blur = new BMenuItem("Blur this Image", new BMessage(menu_view_blur2), '2');
	menu->AddItem(menuitem_view_blur);
	menuitem_view_blur_more = new BMenuItem("Blur this Image More", new BMessage(menu_view_blur3), '3');
	menu->AddItem(menuitem_view_blur_more);

	menubar->AddItem(menu);	

	//Add TV menu
	TV_menu = new BMenu("TV");
	menuitem_TV_monitoring = new BMenuItem("Monitor", new BMessage(menu_TV_monitoring), 'M');
	TV_menu->AddItem(menuitem_TV_monitoring);

	TV_menu->AddSeparatorItem();
	// Build the channel items
	attr_info	infos;
	bool		presets_ok = false;
	BNode		node ("/boot/home/config/settings/stampTV.preferences");
	if (node.InitCheck () == B_OK && node.GetAttrInfo("Presets", &infos) == B_OK) {
		char* buffer = new char[infos.size];
		BMessage	settings;
		if (node.ReadAttr("Presets", infos.type, 0, buffer, infos.size) == infos.size
			&& settings.Unflatten(buffer) == B_OK && settings.what == 'Pref') {
			// presets loaded normaly
			const char*	name;
			int32		channel;
			int			p = 0;
			while (settings.FindString("PresetName", p, &name) == B_OK &&
				settings.FindInt32("PresetChannel", p, &channel) == B_OK) {
				if (channel >= 0) {
					if (!presets_ok) {
						presets_ok = true; // we have at least one channel!
						menuitem = new BMenuItem("Previous Channel", new BMessage(menu_TV_prev_channel), '-');
						TV_menu->AddItem(menuitem);
						menuitem = new BMenuItem("Next Channel", new BMessage(menu_TV_next_channel), '+');
						TV_menu->AddItem(menuitem);
						TV_menu->AddSeparatorItem();
					}
					BMessage * m = new BMessage (menu_TV_channel);
					m->AddString ("name", name);
					m->AddInt32 ("channel", channel);
					menuitem = new BMenuItem(name, m);
					menuitem->SetMessage (m);
					menuitem->SetTarget (be_app_messenger);
					TV_menu->AddItem(menuitem);
				}
				p++;
			}
		}
		delete[] buffer;
	}
	if (!presets_ok) {
		menuitem = new BMenuItem("You could have here a list of channel presets...", new BMessage(B_QUIT_REQUESTED));
		TV_menu->AddItem(menuitem);
		menuitem->SetEnabled (false);
		menuitem = new BMenuItem("Use stampTV for that!", new BMessage(B_QUIT_REQUESTED));
		TV_menu->AddItem(menuitem);
		menuitem->SetEnabled (false);
	}	

	menubar->AddItem(TV_menu);	
	AddChild(menubar);

	BRect	fr (0, menubar->Bounds().Height()+1, Frame().Width(), Frame().Height());
	fr.right -= B_V_SCROLL_BAR_WIDTH;
	fr.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	view = new View (fr);
	AddChild (new BScrollView ("scrollview", view, B_FOLLOW_ALL_SIDES, B_WILL_DRAW, true, true));
	RedoSizes();

	SetPulseRate(200000);
	Show ();
}

void
Window::MessageReceived(BMessage * msg)
{
	switch(msg->what){
	case B_REFS_RECEIVED:
	case B_SIMPLE_DATA:
		if (!view->while_processing) SetSourceImage (msg, false);
		break;	
	case menu_file_opensource:
		if (!view->while_processing) {
			BFilePanel * load = new BFilePanel(B_OPEN_PANEL, NULL, 0, 0, false,
					new BMessage (panel_set_source), NULL, true, true);
			load->SetTarget(this);
			load->Show();
		}
		break;
	case menu_file_continue:
		if (!view->while_processing) {
			BFilePanel * load = new BFilePanel(B_OPEN_PANEL, NULL, 0, 0, false, new BMessage (panel_set_output), NULL, true, true);
			load->SetTarget(this);
			BEntry		entry (view->sourcefilename.String ());
			BEntry parent;
			if (entry.InitCheck () == B_OK && entry.GetParent (&parent) == B_OK)
				load->SetPanelDirectory (&parent);
			load->Show();
		}
		break;
	case menu_file_work:
		if (!view->while_processing && view->sourceimage) {
			view->while_processing = true;
			if (!view->outputimage) {
				BBitmap * image = new BBitmap (view->sourceimage->Bounds(), B_RGB32);
				view->SetOutputImage (image, "");
				be_app->PostMessage (start_processing);
			} else
				be_app->PostMessage (start_processing_with_pic);
		}
		break;
	case menu_file_save:
		Menu_File_Save (false);
		break;
	case menu_file_save_as:
		Menu_File_Save (true);
		break;
	case panel_save_output: {
		entry_ref ref;
		const char *name;
		BPath path;
		BEntry entry;
		if (msg->FindRef("directory", &ref) == B_OK
			&& msg->FindString("name", &name) == B_OK
			&& entry.SetTo (&ref) == B_OK
			&& entry.GetPath (&path) == B_OK
			&& path.Append (name) == B_OK)
		{
				view->outputfilename = path.Path();
				be_app->PostMessage (save_result);
		}
		break;
	}
	case panel_set_source:
		if (!view->while_processing) SetSourceImage(msg);
		break;
	case panel_set_output:
		if (!view->while_processing) SetOutputImage(msg);
		break;
	case menu_file_reload_source: {
		BBitmap * image = BTranslationUtils::GetBitmapFile(view->sourcefilename.String());
		if (image && image->Bounds () == view->sourceimage->Bounds ()) {
			delete view->sourceimage;
			view->sourceimage = image;
		} else
			delete image;
	}
	case menu_view_source:
		view->ViewSource();
		break;
	case menu_file_reload_output: {
		BBitmap * image = BTranslationUtils::GetBitmapFile(view->outputfilename.String());
		if (image && image->Bounds () == view->outputimage->Bounds ()) {
			delete view->outputimage;
			view->outputimage = image;
		} else
			delete image;
	}
	case menu_view_output:
		if (view->outputimage) {
			view->ViewOutput();
		}
		break;
	case menu_view_blur1:
	case menu_view_blur2:
	case menu_view_blur3:
		if (view->while_processing)
			be_app->PostMessage (stop_processing);
		view->BlurImage (msg->what);
		break;
	case menu_TV_monitoring:
		monitoring = !monitoring;
		be_app->PostMessage (app_switch_monitoring);
		break;
	case window_check_channel: {
		int32	channel;
		if (msg->FindInt32 ("channel", &channel) == B_OK) {
			BMenuItem *	item;
			for (int i = 0; (item = TV_menu->ItemAt (i)) != NULL; i++) {
				BMessage * m = item->Message ();
				int32	item_channel;
				if (m && m->FindInt32 ("channel", &item_channel) == B_OK)
					item->SetMarked (channel == item_channel);
			}
		}
		break;
	}
	case menu_TV_prev_channel: {
		BMenuItem *	item;
		BMessage *	prev = NULL;
		for (int i = 0; (item = TV_menu->ItemAt (i)) != NULL; i++) {
			BMessage * m = item->Message ();
			int32	item_channel;
			if (m && m->FindInt32 ("channel", &item_channel) == B_OK) {
				if (item->IsMarked ()) {
					if (prev)
						be_app->PostMessage (prev);
					break;
				} else
					prev = m;
			}
		}
		break;
	}
	case menu_TV_next_channel: {
		BMenuItem *	item;
		bool		select_next = false;
		for (int i = 0; (item = TV_menu->ItemAt (i)) != NULL; i++) {
			BMessage * m = item->Message ();
			int32	item_channel;
			if (m && m->FindInt32 ("channel", &item_channel) == B_OK) {
				if (item->IsMarked ())
					select_next = true;
				else if (select_next) {
					be_app->PostMessage (m);
					break;
				}
			}
		}
		break;
	}
	default:
		BWindow::MessageReceived(msg);
	}
}

bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

void
Window::MenusBeginning()
{
	menuitem_view_open->SetEnabled (!view->while_processing);
	menuitem_view_reload_source->SetEnabled (view->sourceimage && !view->while_processing);
	menuitem_view_continue->SetEnabled (view->sourceimage && !view->while_processing);
	menuitem_view_reload_output->SetEnabled (view->outputimage && !view->while_processing && view->outputfilename.Length() > 2);
	menuitem_view_start->SetEnabled (view->sourceimage && !view->while_processing);
	menuitem_view_stop->SetEnabled (view->while_processing);
	menuitem_view_save->SetEnabled (view->outputimage && view->needSaving);
	menuitem_view_save_as->SetEnabled (view->outputimage);
	menuitem_view_source->SetEnabled (view->sourceimage);
	menuitem_view_source->SetMarked (view->viewimage && view->viewimage == view->sourceimage);
	menuitem_view_output->SetEnabled (view->outputimage);
	menuitem_view_output->SetMarked(view->viewimage && view->viewimage == view->outputimage);
	menuitem_view_blur_little->SetEnabled (view->viewimage);
	menuitem_view_blur->SetEnabled (view->viewimage);
	menuitem_view_blur_more->SetEnabled (view->viewimage);
	menuitem_TV_monitoring->SetEnabled (view->while_processing);
	menuitem_TV_monitoring->SetMarked (monitoring);
}

void
Window::SetSourceImage(BMessage * msg, bool source_only = true)
{
	BRect	frame;
	if (view->viewimage)
		frame = view->viewimage->Bounds ();
	uint32 type;		//Type of message (Ignore this, we already know it's a B_REFS_RECEIVED)
	int32 count;		//The number of files in the ref
	msg->GetInfo("refs", &type, &count);		//Get the number of files in the ref
	entry_ref ref;
	
	if (count > 0 && msg->FindRef("refs", 0, &ref) == B_OK) {
		BEntry entry(&ref, true);
		BPath path;
		entry.GetPath(&path);
		BBitmap * image = BTranslationUtils::GetBitmapFile(path.Path());
		if (image) {
			if (source_only) {
				view->SetOutputImage ();
				view->SetSourceImage (image, path.Path());
			} else {
				if (!view->sourceimage || image->Bounds () != view->sourceimage->Bounds ()) {
					view->SetOutputImage ();
					view->SetSourceImage (image, path.Path());
				} else {
					BAlert * now_what = new BAlert("Um...", "Should I use this image as a source or is it a result to continue?",
						"Cancel", "Source", "Result to Continue", B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
					now_what->SetShortcut(0, B_ESCAPE);
					int choice = now_what->Go();
					if (choice == 1) {
						view->SetOutputImage ();
						view->SetSourceImage (image, path.Path());
					} else if (choice == 2) {
						view->SetOutputImage (image, path.Path());
						PostMessage (menu_file_work);
					} else
						delete image;
				}
			}
		}
	}
	RedoSizes();
	if (view->viewimage && frame != view->viewimage->Bounds ())
		Zoom ();
}

void
Window::SetOutputImage(BMessage * msg)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	msg->GetInfo("refs", &type, &count);
	if (count > 0 && msg->FindRef("refs", 0, &ref) == B_OK) {
		BEntry entry (&ref, true);
		BPath path;
		entry.GetPath (&path);
		BBitmap * image = BTranslationUtils::GetBitmapFile(path.Path());
		if (image) {
			if (image->Bounds () != view->sourceimage->Bounds ()) {
				BAlert * now_what = new BAlert("Um...", "This image has a different size than the first one...\nWhat do You want?",
					"Cancel", "Use as Source", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
				now_what->SetShortcut(0, B_ESCAPE);
				int choice = now_what->Go();
				if (choice == 1) {
					view->SetOutputImage ();
					view->SetSourceImage (image, path.Path());
					RedoSizes();
					Zoom ();
				} else
					delete image;
			} else {
				view->SetOutputImage (image, path.Path());
			}
		}
	}
}

void
Window::Menu_File_Save(bool newfile)
{
	be_app->PostMessage (stop_processing);
	if (newfile || view->outputfilename.Length () < 2) {
		BFilePanel *  filepanel = new BFilePanel(B_SAVE_PANEL, NULL, 0, 0, false, new BMessage (panel_save_output), NULL, true, true);
		filepanel->SetTarget(this);
		BEntry		entry;
		BString		outputname;
		int	n = 1;
		do {
			outputname = view->sourcefilename;
			outputname << " - " << n++;
			entry.SetTo (outputname.String ());
		} while (entry.Exists ());
		char	name[B_FILE_NAME_LENGTH];
		if (entry.GetName (name) == B_OK)
			filepanel->SetSaveText (name);
		BEntry parent;
		if (entry.GetParent (&parent) == B_OK)
			filepanel->SetPanelDirectory (&parent);
		filepanel->Show();
	} else
		be_app->PostMessage (save_result);
}

void
Window::RedoSizes()
{
	float	maxX, maxY;
	view->GetMaxSize(&maxX, &maxY);
	maxX += 14.;
	maxY += 34.;
	SetSizeLimits(100, maxX, 100, maxY);
	SetZoomLimits(maxX, maxY);
}