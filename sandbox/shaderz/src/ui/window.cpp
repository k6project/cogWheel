#include "shaderz.h"
#include "../callbacks.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>

enum
{
    MENU_FILE,
        MENU_NEW_SHADER,
            MENU_ITEM_NEW_GLSL_VERTEX_SHADER,
            MENU_ITEM_NEW_GLSL_FRAGMENT_SHADER,
            MENU_ITEM_NEW_GLSL_COMPUTE_SHADER,
        MENU_ITEM_OPEN_SHADER,
        MENU_ITEM_SAVE_SHADER,
        MENU_ITEM_QUIT,
    MENU_VIEW,
        MENU_ITEM_SPIRV_DASM,
        MENU_ITEM_SPIRV_CPP,
    MENU_HELP,
        MENI_ITEM_ABOUT,
    MENU_INVALID
};

static const char* const DEFAULT_ENTRY_POINT = "main";

static const char* const DEFAULT_VS_FILENAME = "NoName.vert";
static const char* const DEFAULT_VS
{
    "#version 450\n\n" "out vec4 vPosition;\n\n"
    "layout(location = 0) in vec3 aPosition;\n\n"
    "void main()\n{\n    vPosition = vec4(aPosition, 1.0);\n}\n"
};

static const char* const DEFAULT_FS_FILENAME = "NoName.frag";
static const char* const DEFAULT_FS
{
    "#version 450\n\n" "layout(location = 0) out vec4 vFragColor;\n\n"
    "void main()\n{\n    vFragColor = vec4(1.0);\n}\n"
};

struct shaderzGui_t
{
	Fl_Window* window;
    Fl_Menu_Bar* menuBar;
	Fl_Multiline_Output* messageLog;
	Fl_Multiline_Input* sourceCode;
	Fl_Multiline_Output* resultView;
	int spacing;
};

#ifdef __cplusplus
extern "C"
{
#endif
    
void onCompilationSuccess(const compileJob_t* job)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(job->context);
    gui.resultView->value(job->code, job->length & INT32_MAX);
    gui.menuBar->find_index("");
}
    
#ifdef __cplusplus
}
#endif

void onNewVertMenuItem(Fl_Widget* src, void*)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(src->user_data());
    gui.sourceCode->value(DEFAULT_VS);
    compileJob_t job = {};
    job.code = gui.sourceCode->value();
    job.length = gui.sourceCode->size();
    job.jobType = COMPILE_JOB_SPIRV_ASM;
    job.lang = shaderc_source_language_glsl;
    job.type = shaderc_vertex_shader;
    job.fileName = DEFAULT_VS_FILENAME;
    job.mainProc = DEFAULT_ENTRY_POINT;
    job.onSuccess = &onCompilationSuccess;
    job.context = &gui;
    compileShader(&job);
}

void onNewFragMenuItem(Fl_Widget* src, void*)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(src->user_data());
    gui.sourceCode->value(DEFAULT_FS);
    compileJob_t job = {};
    job.code = gui.sourceCode->value();
    job.length = gui.sourceCode->size();
    job.jobType = COMPILE_JOB_SPIRV_ASM;
    job.lang = shaderc_source_language_glsl;
    job.type = shaderc_fragment_shader;
    job.fileName = DEFAULT_FS_FILENAME;
    job.mainProc = DEFAULT_ENTRY_POINT;
    job.onSuccess = &onCompilationSuccess;
    job.context = &gui;
    compileShader(&job);
}

void onOpenMenuItem(Fl_Widget* src, void*)
{
    Fl_Native_File_Chooser fileDlg;
    fileDlg.title("Open file");
    fileDlg.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fileDlg.filter(
        "Vertex Shader\t*.vert\n"
        "Fragment Shader\t*.frag\n"
        "Compute Shader\t*.comp\n"
    );
    if (fileDlg.show() == 0);
    {
        shaderzGui_t& gui = *static_cast<shaderzGui_t*>(src->user_data());
        gui.messageLog->value(fileDlg.filename());
    }
}

void onExitMenuItem(Fl_Widget*, void*)
{
    exit(0);
}

void initLogOutput(shaderzGui_t& gui)
{
	int x = gui.spacing;
	int w = gui.window->w() - (x << 1);
	int h = gui.window->h() / 5;
	int y = gui.window->h() - h - gui.spacing;
	gui.messageLog = new Fl_Multiline_Output(x, y, w, h);
	gui.messageLog->cursor_color(gui.messageLog->color());
	gui.messageLog->value("Test line 1\n""Test line 2\n");
    gui.messageLog->textfont(FL_COURIER);
    gui.messageLog->textsize(16);
}

void initMainViews(shaderzGui_t& gui)
{
	int x = gui.spacing;
	int wTotal = gui.window->w() - gui.spacing * 3;
    int w = wTotal >> 1;
	int y = gui.spacing + gui.menuBar->h();
	int h = gui.window->h() - gui.messageLog->h() - gui.spacing * 3 - gui.menuBar->h();
	gui.sourceCode = new Fl_Multiline_Input(x, y, w, h);
    gui.sourceCode->textfont(FL_COURIER);
    gui.sourceCode->textsize(16);
	x += w + gui.spacing;
	w = wTotal - w;
	gui.resultView = new Fl_Multiline_Output(x, y, w, h);
	gui.resultView->cursor_color(gui.resultView->color());
    gui.resultView->textfont(FL_COURIER);
    gui.resultView->textsize(16);
}

void initShaderzGui(shaderzGui_t& gui, int x, int y, int w, int h)
{
    Fl_Menu_Item menuItems[]
    {
        {"&File", 0, 0, 0, FL_SUBMENU},
            {"&New Shader...", FL_COMMAND + 'n', 0, 0, FL_SUBMENU},
                {"Vertex Shader", 0, &onNewVertMenuItem},
                {"Fragment Shader", 0, &onNewFragMenuItem},
                {"Compute Shader", 0, 0, 0},
            {0},
            {"&Open Shader...", FL_COMMAND + 'o', &onOpenMenuItem},
            {"&Save Shader", FL_COMMAND + 's', 0, 0, FL_MENU_DIVIDER},
            {"&Quit", FL_COMMAND + 'q', &onExitMenuItem, 0},
        {0},
        {"&View", 0, 0, 0, FL_SUBMENU},
            {"SPIR-V Disassembly", 0, 0, 0, FL_MENU_RADIO},
            {"C/C++ byte array", 0, 0, 0, FL_MENU_RADIO},
        {0},
        {"&Help", 0, 0, 0, FL_SUBMENU},
            {"About ShaderZ", 0, 0, 0},
        {0},
    {0}};
	gui.window = new Fl_Window(x, y, w, h, "ShaderZ GUI");
    gui.window->begin();
	gui.spacing = ((gui.window->h() < gui.window->w()) ? gui.window->h() : gui.window->w()) >> 8;
	gui.spacing = (gui.spacing == 0) ? 2 : gui.spacing;
    gui.menuBar = new Fl_Menu_Bar(0, 0, gui.window->w(), gui.spacing << 3);
    gui.menuBar->copy(menuItems, gui.window);
    gui.menuBar->box(FL_FLAT_BOX);
    gui.menuBar->user_data(&gui);
	initLogOutput(gui);
	initMainViews(gui);
	gui.window->end();
}

int shaderzShowGui(int argc, char** argv)
{
	int screenX, screenY, screenW, screenH;
	Fl::screen_work_area(screenX, screenY, screenW, screenH, 0, 0);
    int windowW = screenW;// - (screenW >> 3);
    int windowH = screenH - screenY;// - (screenH >> 3);
	int windowX = screenX + (screenW >> 4);
    int windowY = screenY;// + (screenH >> 4);
	shaderzGui_t gui = {};
	initShaderzGui(gui, windowX, windowY, windowW, windowH);
	if (gui.window)
	{
		gui.window->show(argc, argv);
		return Fl::run();
	}
	return -1;
}
