#include "gameos.hpp"
#include "gos_render.h"
#include <stdio.h>
#include <time.h>

#include <SDL2/SDL.h>
#include "gos_input.h"

#include "utils/camera.h"
#include "utils/shader_builder.h"
#include "utils/gl_utils.h"
#include "utils/timing.h"

#include <signal.h>

extern void gos_CreateRenderer(graphics::RenderContextHandle ctx_h, graphics::RenderWindowHandle win_h, int w, int h);
extern void gos_DestroyRenderer();
extern void gos_RendererBeginFrame();
extern void gos_RendererEndFrame();
extern void gos_RendererHandleEvents();
extern void gos_RenderUpdateDebugInput();

extern bool gosExitGameOS();

extern bool gos_CreateAudio();
extern void gos_DestroyAudio();

static bool g_exit = false;
static bool g_focus_lost = false;
bool g_debug_draw_calls = false;
static camera g_camera;

input::MouseInfo g_mouse_info;
input::KeyboardInfo g_keyboard_info;

static void handle_key_down( SDL_Keysym* keysym ) {
    switch( keysym->sym ) {
        case SDLK_ESCAPE:
            if(keysym->mod & KMOD_RALT)
                g_exit = true;
            break;
        case 'd':
            if(keysym->mod & KMOD_RALT)
                g_debug_draw_calls = true;
            break;
    }
}

static void process_events( void ) {

    beginUpdateMouseState(&g_mouse_info);

    SDL_Event event;
    while( SDL_PollEvent( &event ) ) {

        if(g_focus_lost) {
            if(event.type != SDL_WINDOWEVENT_FOCUS_GAINED) {
                continue;
            } else {
                g_focus_lost = false;
            }
        }

        switch( event.type ) {
        case SDL_KEYDOWN:
            handle_key_down( &event.key.keysym );
            // fallthrough
        case SDL_KEYUP:
            handleKeyEvent(&event, &g_keyboard_info);
            break;
        case SDL_QUIT:
            g_exit = true;
            break;
		case SDL_WINDOWEVENT_RESIZED:
			{
				float w = (float)event.window.data1;
				float h = (float)event.window.data2;
				glViewport(0, 0, (GLsizei)w, (GLsizei)h);
                SPEW(("INPUT", "resize event: w: %f h:%f\n", w, h));
			}
			break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            g_focus_lost = true;
            break;
        case SDL_MOUSEMOTION:
            input::handleMouseMotion(&event, &g_mouse_info); 
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            //input::handleMouseButton(&event, &g_mouse_info);
            break;
        case SDL_MOUSEWHEEL:
            input::handleMouseWheel(&event, &g_mouse_info);
            break;
        }
    }

    input::updateMouseState(&g_mouse_info);
    input::updateKeyboardState(&g_keyboard_info);
}

extern bool g_disable_quads;

static void draw_screen( void )
{
    g_disable_quads = false;
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glCullFace(GL_FRONT);
    glEnable(GL_TEXTURE_2D);
    //CHECK_GL_ERROR;
    
	const int viewport_w = Environment.drawableWidth;
	const int viewport_h = Environment.drawableHeight;
    glViewport(0, 0, viewport_w, viewport_h);

    mat4 proj;
    g_camera.get_projection(&proj);
    mat4 viewM;
    g_camera.get_view(&viewM);

#if 0
    gos_VERTEX q[4];
    q[0].x = 10; q[0].y = 10;
    q[0].z = 0.0;
    q[0].rhw = 1;
    q[0].argb = 0xffff0000;
    q[1] = q[2] = q[3] = q[0];

    q[1].x = 210; q[1].y = 10;
    q[2].x = 110; q[2].y = 210;
    q[3].x = 10; q[3].y = 210;

    g_disable_quads = false;
    gos_DrawQuads(&q[0], 4);
    g_disable_quads = true;
#endif

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadTransposeMatrixf((const float*)proj);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadTransposeMatrixf((const float*)viewM);

    // TODO: reset all states to sane defaults!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    glDepthMask(GL_TRUE);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    gos_RendererBeginFrame();
    Environment.UpdateRenderers();
    gos_RendererEndFrame();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glUseProgram(0);
    //CHECK_GL_ERROR;
}

extern float frameRate;

#ifndef DISABLE_GAMEOS_MAIN
int main(int argc, char** argv)
{
    //signal(SIGTRAP, SIG_IGN);

    // gather command line
    uint32_t cmdline_len = 0;
    for(int i=0;i<argc;++i) {
        cmdline_len += strlen(argv[i]);
        cmdline_len += 1; // ' '
    }
    char* cmdline = new char[cmdline_len + 1];
    uint32_t offset = 0;
    for(int i=0;i<argc;++i) {
        uint32_t arglen = strlen(argv[i]);
        memcpy(cmdline + offset, argv[i], arglen);
        cmdline[offset + arglen] = ' ';
        offset += arglen + 1;
    }
    cmdline[cmdline_len] = '\0';

    // fills in Environment structure
    GetGameOSEnvironment(cmdline);

    delete[] cmdline;
    cmdline = NULL;

    int w = Environment.screenWidth;
    int h = Environment.screenHeight;

    graphics::RenderWindowHandle win = graphics::create_window("mc2", w, h);
    if(!win)
        return 1;

    graphics::RenderContextHandle ctx = graphics::init_render_context(win);
    if(!ctx)
        return 1;

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        SPEW(("GLEW", "Error: %s\n", glewGetErrorString(err)));
        return 1;
    }

    SPEW(("GRAPHICS", "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION)));
    if (!GLEW_ARB_vertex_program || !GLEW_ARB_fragment_program)
    {
        SPEW(("GRAPHICS", "No shader program support\n"));
        return 1;
    }

    if(!glewIsSupported("GL_VERSION_3_0")) {
        SPEW(("GRAPHICS", "Minimum required OpenGL version is 3.0\n"));
        return 1;
    }

    const char* glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    SPEW(("GRAPHICS", "GLSL version supported: %s\n", glsl_version));

    int glsl_maj = 0, glsl_min = 0;
    sscanf(glsl_version, "%d.%d", &glsl_maj, &glsl_min);

    if(glsl_maj < 3 || (glsl_maj==3 && glsl_min < 30) ) {
        SPEW(("GRAPHICS", "Minimum required OpenGL version is 330 ES, current: %d.%d\n", glsl_maj, glsl_min));
        return 1;
    }

    char version[16] = {0};
    snprintf(version, sizeof(version), "%d%d", glsl_maj, glsl_min);
    SPEW(("GRAPHICS", "Using %s shader version\n", version));

    gos_CreateRenderer(ctx, win, w, h);
    if(!gos_CreateAudio())
    {   // not an error
        SPEW(("AUDIO", "Failed to create audio\n"));
    }

    Environment.InitializeGameEngine();

    graphics::make_current_context(ctx);

	float aspect = (float)w/(float)h;
	mat4 proj_mat = frustumProjMatrix(-aspect*0.5f, aspect*0.5f, -.5f, .5f, 1.0f, 100.0f);
	g_camera.set_projection(proj_mat);
	g_camera.set_view(mat4::translation(vec3(0, 0, -16)));

	timing::init();

    while( !g_exit ) {

		uint64_t start_tick = timing::gettickcount();
		timing::sleep(10*1000000);

        if(g_debug_draw_calls) {
            gos_RenderUpdateDebugInput();
        } else {
            Environment.DoGameLogic();
        }

        process_events();

		gos_RendererHandleEvents();

        graphics::make_current_context(ctx);
        draw_screen();
        graphics::swap_window(win);

        g_exit |= gosExitGameOS();

		uint64_t end_tick = timing::gettickcount();
		uint64_t dt = timing::ticks2ms(end_tick - start_tick);
		frameRate = 1000.0f / (float)dt;
    }
    
    Environment.TerminateGameEngine();

    gos_DestroyRenderer();

    graphics::destroy_render_context(ctx);
    graphics::destroy_window(win);

    gos_DestroyAudio();

    return 0;
}
#endif // DISABLE_GAMEOS_MAIN
