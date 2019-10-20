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

int main(void)
{
        setup_window();
        setup_gfx();

        setup_shapesrender();

        add_ellipse(&(struct Ellipse){ -0.2f, 0.3f, 0.2f, -0.3f, 1.0f});

        while (!shouldWindowClose) {
                wait_for_events();
                process_events();
                draw_shapes();
                swap_buffers();
        };        

        return 0;
}