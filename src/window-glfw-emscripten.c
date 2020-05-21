#include <shapes/shapes.h>
#include <shapes/window.h>
#include <shapes/logging.h>
#include <GL/glfw.h>

static const struct {
        int glfwKey;
        int keyKind;
} keymap[] = {
        { GLFW_KEY_ENTER,     KEY_ENTER       },
        { GLFW_KEY_TAB,       KEY_TAB         },
        { GLFW_KEY_UP,        KEY_CURSORUP    },
        { GLFW_KEY_DOWN,      KEY_CURSORDOWN  },
        { GLFW_KEY_LEFT,      KEY_CURSORLEFT  },
        { GLFW_KEY_RIGHT,     KEY_CURSORRIGHT },
        { GLFW_KEY_PAGEUP,   KEY_PAGEUP      },
        { GLFW_KEY_PAGEDOWN, KEY_PAGEDOWN    },
        { GLFW_KEY_HOME,      KEY_HOME        },
        { GLFW_KEY_END,       KEY_END         },
        { GLFW_KEY_ESC,    KEY_ESCAPE      },
        { GLFW_KEY_F1,        KEY_F1 },
        { GLFW_KEY_F2,        KEY_F2 },
        { GLFW_KEY_F3,        KEY_F3 },
        { GLFW_KEY_F4,        KEY_F4 },
        { GLFW_KEY_F5,        KEY_F5 },
        { GLFW_KEY_F6,        KEY_F6 },
        { GLFW_KEY_F7,        KEY_F7 },
        { GLFW_KEY_F8,        KEY_F8 },
        { GLFW_KEY_F9,        KEY_F9 },
        { GLFW_KEY_F10,       KEY_F10 },
        { GLFW_KEY_F11,       KEY_F11 },
        { GLFW_KEY_F12,       KEY_F12 },
        { GLFW_KEY_KP_ADD,      KEY_PLUS },
        { GLFW_KEY_KP_SUBTRACT, KEY_MINUS },
};

static const struct {
        int glfwMousebutton;
        enum MousebuttonKind mousebutton;
} glfwMousebuttonToMousebutton[] = {
        { GLFW_MOUSE_BUTTON_1, MOUSEBUTTON_1 },
        { GLFW_MOUSE_BUTTON_2, MOUSEBUTTON_2 },
        { GLFW_MOUSE_BUTTON_3, MOUSEBUTTON_3 },
        { GLFW_MOUSE_BUTTON_4, MOUSEBUTTON_4 },
        { GLFW_MOUSE_BUTTON_5, MOUSEBUTTON_5 },
        { GLFW_MOUSE_BUTTON_6, MOUSEBUTTON_6 },
        { GLFW_MOUSE_BUTTON_7, MOUSEBUTTON_7 },
        { GLFW_MOUSE_BUTTON_8, MOUSEBUTTON_8 },
};

static const struct {
        int glfwAction;
        enum KeyEventKind keyeventKind;
} glfwActionToKeyeventKind[] = {
        { GLFW_PRESS, KEYEVENT_PRESS },
        //{ GLFW_REPEAT, KEYEVENT_REPEAT },
        { GLFW_RELEASE, KEYEVENT_RELEASE },
};

static int windowGlfw;
static int isFullscreenMode;
static volatile int isDoingPolling;  // needed for a hack. See below


static int glfw_get_modifiers(void)
{
        int modifiers = 0;
        if (glfwGetKey(GLFW_KEY_LALT) || glfwGetKey(GLFW_KEY_RALT))
                modifiers |= MODIFIER_MOD;
        if (glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL))
                modifiers |= MODIFIER_CONTROL;
        if (glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT))
                modifiers |= MODIFIER_SHIFT;
        return modifiers;
}

static void enqueue_key_input(
        enum KeyKind keyKind, enum KeyEventKind keyEventKind,
        int modifierMask, int hasCodepoint, unsigned codepoint)
{
        struct Input inp;
        inp.inputKind = INPUT_KEY;
        inp.data.tKey.keyKind = keyKind;
        inp.data.tKey.keyEventKind = keyEventKind;
        inp.data.tKey.modifierMask = modifierMask;
        inp.data.tKey.hasCodepoint = hasCodepoint;
        inp.data.tKey.codepoint = codepoint;
        enqueue_input(&inp);
}

#include <stdlib.h>
static void mouse_cb_glfw(int button, int action)
{
        enum MousebuttonEventKind mousebuttonEventKind;

        mousebuttonEventKind = action ? MOUSEBUTTONEVENT_PRESS : MOUSEBUTTONEVENT_RELEASE;

        for (int i = 0; i < LENGTH(glfwMousebuttonToMousebutton); i++) {
                if (button == glfwMousebuttonToMousebutton[i].glfwMousebutton) {
                        enum MousebuttonKind mousebuttonKind = glfwMousebuttonToMousebutton[i].mousebutton;
                        struct Input inp;
                        inp.inputKind = INPUT_MOUSEBUTTON;
                        inp.data.tMousebutton.mousebuttonKind = mousebuttonKind;
                        inp.data.tMousebutton.mousebuttonEventKind = mousebuttonEventKind;
                        inp.data.tMousebutton.modifiers = glfw_get_modifiers();
                        enqueue_input(&inp);
                }
        }
}

static void cursor_cb_glfw(int x, int y)
{
        struct Input inp;
        inp.inputKind = INPUT_CURSORMOVE;
        inp.data.tCursormove.pixelX = x;
        inp.data.tCursormove.pixelY = y;
        enqueue_input(&inp);
}

static void key_cb_glfw(int key, int scancode)
{
        for (int i = 0; i < LENGTH(keymap); i++) {
                if (key == keymap[i].glfwKey) {
                        enum KeyKind keyKind = keymap[i].keyKind;
                        enum KeyEventKind keyeventKind = -1;
                        for (int j = 0; j < LENGTH(glfwActionToKeyeventKind); j++)
                                keyeventKind = glfwActionToKeyeventKind[j].keyeventKind;
                        ENSURE(keyeventKind != -1);
                        int modifiers = glfw_get_modifiers();
                        int hasCodepoint = 0;
                        unsigned codepoint = 0;
                        enqueue_key_input(keyKind, keyeventKind, modifiers, hasCodepoint, codepoint);
                        return;
                }
        }
}

static void windowsize_cb_glfw(int win, int width, int height)
{
        UNUSED(win);
        ENSURE(win == windowGlfw);
        if (!width && !height) {
                /* for now, this is our weird workaround against division
                by 0 / NaN problems resulting from window minimization. */
                return;
        }
        windowWidthInPixels = width;
        windowHeightInPixels = height;

        struct Input inp;
        inp.inputKind = INPUT_WINDOWRESIZE;
        inp.data.tWindowresize.width = width;
        inp.data.tWindowresize.height = height;
        enqueue_input(&inp);

        //XXX: there's an issue that glfwPollEvents() blocks on some platforms
        // during a window move or resize (see notes in GLFW docs).
        // To work around that, for now we just duplicate drawing in this callback.
        // it's not nice and likely to break, so consider it a temporary hack.
        /*
        if (isDoingPolling) {
                extern long long timeSinceProgramStartupMilliseconds;
                extern void mainloop(void);
                if (timeSinceProgramStartupMilliseconds > 2000) { // XXX OpenGL should be set up by now
                        mainloop();
                }
        }
        */
}

void enter_windowing_mode(void)
{
#if 0
        GLFWmonitor *monitor = NULL;
        int pixelsW = 800;
        int pixelsH = 600;
        glfwSetWindowMonitor(windowGlfw, monitor, 100, 100, pixelsW, pixelsH, GLFW_DONT_CARE);
        isFullscreenMode = 0;
#endif
}

void enter_fullscreen_mode(void)
{
#if 0
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();  // try full screen mode. Will stuttering go away?
        const struct GLFWvidmode *mode = glfwGetVideoMode(monitor);
        int pixelsW = mode->width;
        int pixelsH = mode->height;
        glfwSetWindowMonitor(windowGlfw, monitor, 0, 0, pixelsW, pixelsH, GLFW_DONT_CARE);
        isFullscreenMode = 1;
#endif
}

void toggle_fullscreen(void)
{
        if (isFullscreenMode)
                enter_windowing_mode();
        else
                enter_fullscreen_mode();
}

void setup_window(void)
{
        if (!glfwInit())
                fatal("Failed to initialize glfw!\n");

        windowGlfw = glfwOpenWindow(1024, 768, 8, 8, 8, 8, 24, 16, GLFW_WINDOW);
        if (!windowGlfw)
                fatal("Failed to create GLFW window\n");
        //enter_fullscreen_mode();
        enter_windowing_mode();

        glfwSetMouseButtonCallback(&mouse_cb_glfw);
        glfwSetMousePosCallback(&cursor_cb_glfw);
        glfwSetKeyCallback(&key_cb_glfw);

        { /* call the callback artificially */
                int pixX, pixY;
                glfwGetWindowSize(&pixX, &pixY);
                windowsize_cb_glfw(windowGlfw, pixX, pixY);
        }
}

void teardown_window(void)
{
        glfwCloseWindow();
        glfwTerminate();
}

void wait_for_events(void)
{
        isDoingPolling = 1;
        // TODO: Use glfwPollEvents() if it's clear that we should produce the next frame immediately
        if (0) {
                glfwWaitEvents();
        }
        else {
                glfwPollEvents();
        }
        isDoingPolling = 0;

        // for now, also generate a virtual tick event
        {
                struct Input inp;
                inp.inputKind = INPUT_TIMETICK;
                enqueue_input(&inp);
        }
}

void swap_buffers(void)
{
        glfwSwapInterval(1);
        glfwSwapBuffers();
}

ANY_FUNCTION *get_OpenGL_function_pointer(const char *name)
{
        return glfwGetProcAddress(name);
}
