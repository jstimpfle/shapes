#include <shapes/window.h>
#include <shapes/gfxrender.h>
#include <shapes/shapes.h>

static void process_events(void)
{
        struct Input input;
        while (look_input(&input)) {
                consume_input();
                if (input.inputKind == INPUT_KEY &&
                        input.data.tKey.keyEventKind == KEYEVENT_PRESS &&
                        input.data.tKey.keyKind == KEY_ESCAPE)
                        shouldWindowClose = 1;
                else {
                        update_shapes(input);
                }
        }
}

static void one_loop_iteration(void)
{
        wait_for_events();
        process_events();
        draw_shapes();
        swap_buffers();
}

int main(void)
{
        setup_window();
        setup_gfx();
        setup_shapes();
        setup_shapesrender();

        Object c0 = add_circle(0.3f, 0.2f, 0.05f);
        Object c1 = add_circle(0.7f, 0.5f, 0.05f);
        add_ellipse(c0, c1, 1.0f);

#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(&one_loop_iteration, 30, 1);
#else
        while (!shouldWindowClose)
                one_loop_iteration();
#endif

        return 0;
}
