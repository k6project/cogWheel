#include <string.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <core/math.h>
#include <core/coredefs.h>
#include <gfx/coredefs.h>

#include "patterns.h"

void onWindowResized(GLFWwindow* window, int width, int height)
{
}

int main(int argc, const char * argv[])
{
    vec3f_t dir = {1.f, 0.f, 0.f};
    vec3f_t axis = {0.f, 0.f, 1.f};
    quat_t rot;
    mathQuatInit(rot, axis, mathDeg2Rad(90));
    mathQuatVec3(dir, rot, dir);
    int test[3] = {1, test[0]+1, test[1]};
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
			gfxApi_t gfx = gfxApi();
            ENSURE(gfx->initDevice(glfwGetNativeView(window)) == GFX_SUCCESS);
            gfxTexture_t tex = gfx->newTexture();
			tex->width = width;
			tex->height = height;
            tex->renderTarget = true;
            /*checkerboard(&tex);*/
            voronoiNoise(tex, 6, 6);
            ENSURE(gfx->initTexture(tex) == GFX_SUCCESS);
			glfwSetFramebufferSizeCallback(window, &onWindowResized);
            while (!glfwWindowShouldClose(window))
            {
				gfx->beginFrame();
				gfx->update(&tex, 1, NULL, 0);
				gfx->clear(NULL, (vec4f_t) { 0.f, 0.2f, 1.f, 1.f });
				gfx->blit(NULL, tex);
				gfx->endFrame();
                glfwPollEvents();
            }
			gfx->destroyTexture(tex);
            gfx->destroyDevice();
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    return 0;
}
