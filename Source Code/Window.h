/*

	Window.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000, Georges-Edouard Berenger, All Rights Reserved.

*/

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <Window.h>

class View;

class Window : public BWindow
{
public:
	Window();
	virtual void	MessageReceived(BMessage * msg);	
	virtual bool	QuitRequested();
	virtual	void	MenusBeginning();
	
	View * 			view;
	bool			monitoring;

private:
	void			SetSourceImage(BMessage * msg, bool source_only = true);
	void			SetOutputImage(BMessage * msg);
	
	void			Menu_File_Save(bool newfile);

	void			RedoSizes();

	BMenuItem *		menuitem_view_open;
	BMenuItem *		menuitem_view_reload_source;
	BMenuItem *		menuitem_view_continue;
	BMenuItem *		menuitem_view_reload_output;

	BMenuItem *		menuitem_view_start;
	BMenuItem *		menuitem_view_stop;

	BMenuItem *		menuitem_view_save;
	BMenuItem *		menuitem_view_save_as;

	BMenuItem *		menuitem_view_source;
	BMenuItem *		menuitem_view_output;

	BMenuItem *		menuitem_view_blur_little;
	BMenuItem *		menuitem_view_blur;
	BMenuItem *		menuitem_view_blur_more;

	BMenuItem *		menuitem_TV_monitoring;
	
	BMenu *			TV_menu;
};

#endif // _WINDOW_H_
