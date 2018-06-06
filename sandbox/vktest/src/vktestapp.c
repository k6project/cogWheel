#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif
#include <GLFW/glfw3.h>

#include <core/math.h>
#include <core/coredefs.h>

#include "renderer.h"
#include "patterns.h"

int main(int argc, const char * argv[])
{
    vklInitialize(*argv);
    if (glfwInit())
    {
        GLFWmonitor* monitor = NULL;
        int width = 768, height = 768;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef PROGRAM_FULLSCREEN
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width;
        height = vidMode->height;
#endif
        GLFWwindow* window = glfwCreateWindow(width, height, PROGRAM_NAME, monitor, NULL);
        if (window)
        {
			gfxDevice_t gfx = gfxAllocDevice();
            ENSURE(gfxCreateDevice(gfx, window) == VK_SUCCESS);
            gfxTexture_t tex = gfxAllocTexture(gfx);
			tex->width = width;
			tex->height = height;
            tex->renderTarget = true;
            /*checkerboard(&tex);*/
            voronoiNoise(tex, 6, 6);
            ENSURE(gfxCreateTexture(gfx, tex) == VK_SUCCESS);
            while (!glfwWindowShouldClose(window))
            {
                gfxBeginFrame(gfx);
                gfxUpdateResources(gfx, &tex, 1, NULL, 0);
                gfxClearRenderTarget(gfx, NULL, (vec4f_t){0.f, 0.2f, 1.f, 1.f});
				gfxBlitTexture(gfx, NULL, tex);
                gfxEndFrame(gfx);
                glfwPollEvents();
            }
			gfxDestroyTexture(gfx, tex);
            gfxDestroyDevice(gfx);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    vklShutdown();
    return 0;
}
