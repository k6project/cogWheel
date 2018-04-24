#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif
#include <GLFW/glfw3.h>

#include <core/math.h>

#include "renderer.h"
#include "patterns.h"


/* TEST APPLICATION CODE */

/*
 
 vulkan10.h - loader and helper functions for vulkan library
 vulkan10.c
 
 renderer.h
 renderer.c - application-specific implementation (can be copied over)
 
 spirvlib.h
 spirvlib.c - app-specific shaders serialized as arrays
 
 frame flow: if upload job is staged, perform it, otherwise draw graphics
             or use parallel queues to submit both
 
 initial values for memory budgets: 64 mb staging (cpu coherent), 192 mb device local

 forward pass: color0 (offscreen) form undefined to transfer_src_optimal
               color1 (backbuffer) from undefined to transfer_dst_optimal
 subpass 0:color0 is color_attachment_optimal, color1 is not used
 
 after forward pass use image memory barrier to transfer to present_src

 */

/*
 
 gfxBeginFrame(&gfx);//wait for present queue to finish and acquire image
 .....
 gfxResolveMSAA(&gfx);
 .....
 <ui rendering>//
 .....
 gfxSubmitFrame(&gfx); //blit to framebuffer and submit
 
 */


int main(int argc, const char * argv[])
{
    vklInitialize(*argv);
    if (glfwInit())
    {
        GLFWmonitor* monitor = NULL;
        int width = 1280, height = 800;
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
			gfxContext_t gfx;
            memset(&gfx, 0, sizeof(gfx));
            assert(gfxCreateDevice(&gfx, window) == VK_SUCCESS);
			gfxTexture_t tex;
			memset(&tex, 0, sizeof(tex));
			tex.width = width;
			tex.height = height;
            checkerboard(&tex);
			assert(gfxCreateTexture(&gfx, &tex) == VK_SUCCESS);
            while (!glfwWindowShouldClose(window))
            {
                gfxBeginFrame(&gfx);
				if (tex.hasPendingData)
				{
                    memcpy(gfx.stagingBuffer.hostPtr, tex.imageData, tex.imageDataSize);
                    /*stagedata (copy)*/
                    
					/*upload data via staging buffer*/
					/* each upload request copies to a portion of staging buffer and adds a copy command to a list */
					/* when staging buffer is full: if more uploads pending, blit a copy of previous frame, otherwise draw */
				}
                /*blit to gfx->backBuffer (transition resources, do blit)*/
                /*endframe (transition backbuffer to present)*/
                gfxEndFrame(&gfx);
                glfwPollEvents();
            }
			gfxDestroyTexture(&gfx, &tex);
            gfxDestroyDevice(&gfx);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    vklShutdown();
    return 0;
}
