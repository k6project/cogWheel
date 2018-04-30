#include "shaderz.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Multiline_Output.H>

struct shaderzGui_t
{
	Fl_Window* window;
	Fl_Multiline_Output* messageLog;
	Fl_Multiline_Output* sourceCode;
	Fl_Multiline_Output* resultView;
	int spacing;
};

void initLogOutput(shaderzGui_t& gui)
{
	int x = gui.spacing;
	int w = gui.window->w() - (x << 1);
	int h = gui.window->h() / 5;
	int y = gui.window->h() - h - gui.spacing;
	gui.messageLog = new Fl_Multiline_Output(x, y, w, h);
	gui.messageLog->cursor_color(gui.messageLog->color());
	gui.messageLog->value("Test line 1\n""Test line 2\n");
}

void initMainViews(shaderzGui_t& gui)
{
	int x = gui.spacing;
	int wTotal = gui.window->w() - gui.spacing * 3;
	int w = (wTotal / 3) << 1;
	int y = gui.spacing;
	int h = gui.window->h() - gui.messageLog->h() - gui.spacing * 3;
	gui.sourceCode = new Fl_Multiline_Output(x, y, w, h);
	gui.sourceCode->cursor_color(gui.sourceCode->color());
	x += w + gui.spacing;
	w = wTotal - w;
	gui.resultView = new Fl_Multiline_Output(x, y, w, h);
	gui.resultView->cursor_color(gui.resultView->color());
}

void initShaderzGui(shaderzGui_t& gui, int x, int y, int w, int h)
{
	gui.window = new Fl_Window(x, y, w, h, "ShaderZ GUI");
	gui.spacing = ((gui.window->h() < gui.window->w()) ? gui.window->h() : gui.window->w()) >> 8;
	gui.spacing = (gui.spacing == 0) ? 2 : gui.spacing;
	gui.window->size_range(w, h);
	initLogOutput(gui);
	initMainViews(gui);
	gui.window->end();
}

int shaderzShowGui(int argc, char** argv)
{
	int screenId = Fl::screen_num(0, 0);
	int screenX, screenY, screenW, screenH;
	Fl::screen_work_area(screenX, screenY, screenW, screenH, 0, 0);
	int windowW = screenW - (screenW >> 3);
	int windowH = screenH - (screenH >> 3);
	int windowX = screenX + (screenW >> 4);
	int windowY = screenY + (screenH >> 4);
	shaderzGui_t gui = {};
	initShaderzGui(gui, windowX, windowY, windowW, windowH);
	if (gui.window)
	{
		gui.window->show(argc, argv);
		return Fl::run();
	}
	return -1;
}
