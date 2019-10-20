#include <shapes/defs.h>

struct Ellipse {
        float centerX0;
        float centerX1;
        float centerY0;
        float centerY1;
        float radius;
};

struct Circle {
        float centerX;
        float centerY;
        float radius;
};

DATA struct Ellipse *ellipseShapes;
DATA struct Circle *circleShapes;

DATA int numEllipses;
DATA int numCircles;


void setup_shapesrender(void);
void draw_shapes(void);

void add_ellipse(const struct Ellipse *ellipse);
void add_circle(const struct Circle *circle);
#include <shapes/window.h>
void update_shapes(struct Input input);