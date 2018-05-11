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
    MENU_FILE_BEGIN,
        MENU_NEW_SHADER_BEGIN,
            MENU_ITEM_NEW_GLSL_VERTEX_SHADER,
            MENU_ITEM_NEW_GLSL_FRAGMENT_SHADER,
            MENU_ITEM_NEW_GLSL_COMPUTE_SHADER,
		MENU_NEW_SHADER_END,
        MENU_ITEM_OPEN_SHADER,
        MENU_ITEM_SAVE_SHADER,
        MENU_ITEM_QUIT,
	MENU_FILE_END,
    MENU_VIEW_BEGIN,
        MENU_ITEM_SPIRV_DASM,
        MENU_ITEM_SPIRV_CPP,
	MENU_VIEW_END,
    MENU_HELP_BEGIN,
        MENI_ITEM_ABOUT,
	MENU_HELP_END,
    MENU_INVALID
};

static const char* const DEFAULT_ENTRY_POINT = "main";

static const char* const DEFAULT_VS_FILENAME = "NoName.vert";
static const char* const DEFAULT_VS
{
    "#version 450\n\n"
    "layout(location = 0) in vec3 aPosition;\n\n"
    "void main()\n{\n    gl_Position = vec4(aPosition, 1.0);\n}\n"
};

static const char* const DEFAULT_FS_FILENAME = "NoName.frag";
static const char* const DEFAULT_FS
{
    "#version 450\n\n" "layout(location = 0) out vec4 vFragColor;\n\n"
    "void main()\n{\n    vFragColor = vec4(1.0);\n}\n"
};

static const char* const DEFAULT_CS_FILENAME = "NoName.comp";
static const char* const DEFAULT_CS
{
    "#version 450\n\n" "void main()\n{\n\n}\n"
};

struct shaderzGui_t
{
	Fl_Window* window;
    Fl_Menu_Bar* menuBar;
	Fl_Multiline_Output* messageLog;
	Fl_Multiline_Input* sourceCode;
	Fl_Multiline_Output* resultView;
    const char* fileName;
    shaderc_shader_kind currType : 32;
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
	Fl_Menu_Item& item = const_cast<Fl_Menu_Item&>(gui.menuBar->menu()[MENU_ITEM_SPIRV_DASM]);
	gui.menuBar->setonly(&item);
	Fl::copy(job->code, job->length & INT32_MAX);
}

void onCompilationError(const compileJob_t* job)
{
	shaderzGui_t& gui = *static_cast<shaderzGui_t*>(job->context);
	gui.messageLog->value(job->code, job->length & INT32_MAX);
}
    
#ifdef __cplusplus
}
#endif

void onCompileShader(Fl_Widget*, void* ctx)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(ctx);
    compileJob_t job = {};
    if (gui.menuBar->menu()[MENU_ITEM_SPIRV_DASM].value())
    {
        job.jobType = COMPILE_JOB_SPIRV_ASM;
    }
    else if (gui.menuBar->menu()[MENU_ITEM_SPIRV_CPP].value())
    {
        job.jobType = COMPILE_JOB_SPIRV_CPP;
    }
    job.code = gui.sourceCode->value();
    job.length = gui.sourceCode->size();
    job.lang = shaderc_source_language_glsl;
    job.type = gui.currType;
    job.fileName = gui.fileName;
    job.mainProc = DEFAULT_ENTRY_POINT;
    job.onSuccess = &onCompilationSuccess;
    job.onError = &onCompilationError;
    job.context = ctx;
    compileShader(&job);
}

void onNewVertMenuItem(Fl_Widget* src, void* ctx)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(ctx);
    gui.currType = shaderc_vertex_shader;
    gui.fileName = DEFAULT_VS_FILENAME;
    gui.sourceCode->value(DEFAULT_VS);
    onCompileShader(src, ctx);
}

void onNewFragMenuItem(Fl_Widget* src, void* ctx)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(ctx);
    gui.currType = shaderc_fragment_shader;
    gui.fileName = DEFAULT_FS_FILENAME;
    gui.sourceCode->value(DEFAULT_FS);
    onCompileShader(src, ctx);
}

void onNewCompMenuItem(Fl_Widget* src, void* ctx)
{
    shaderzGui_t& gui = *static_cast<shaderzGui_t*>(ctx);
    gui.currType = shaderc_compute_shader;
    gui.fileName = DEFAULT_CS_FILENAME;
    gui.sourceCode->value(DEFAULT_CS);
    onCompileShader(src, ctx);
}

void onOpenMenuItem(Fl_Widget*, void* ctx)
{
    Fl_Native_File_Chooser fileDlg;
    fileDlg.title("Open file");
    fileDlg.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fileDlg.filter(
        "Vertex Shader\t*.vert\n"
        "Fragment Shader\t*.frag\n"
        "Compute Shader\t*.comp\n"
    );
    if (fileDlg.show() == 0)
    {
        shaderzGui_t& gui = *static_cast<shaderzGui_t*>(ctx);
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
    gui.messageLog->textfont(FL_COURIER);
    gui.messageLog->textsize(16);
}

void initMainViews(shaderzGui_t& gui)
{
	int x = gui.spacing;
	int wTotal = gui.window->w() - gui.spacing * 3;
    int w = wTotal >> 1;
	int y = gui.menuBar->h();
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
                {"Compute Shader", 0, &onNewCompMenuItem},
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
	gui.spacing = ((gui.window->h() < gui.window->w()) ? gui.window->h() : gui.window->w()) >> 8;
	gui.spacing = (gui.spacing == 0) ? 2 : gui.spacing;
    gui.menuBar = new Fl_Menu_Bar(0, 0, gui.window->w(), 24);
    menuItems[MENU_ITEM_SPIRV_CPP].set();
    gui.menuBar->copy(menuItems, &gui);
    gui.menuBar->box(FL_FLAT_BOX);
    gui.menuBar->user_data(&gui);
	gui.menuBar->textsize(12);
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
