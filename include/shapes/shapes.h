#include <shapes/defs.h>
#include <shapes/geometry.h>
#include <shapes/window.h> // struct Input

enum {
        OBJECT_CIRCLE,
        OBJECT_ELLIPSE,
};

typedef int Object;

struct Circle {
        float centerX;
        float centerY;
        float radius;
};

struct Ellipse {
        Object centerCircle0;
        Object centerCircle1;
        float radius;
};

struct Object {
        int objectKind;
        union {
                struct Circle tCircle;
                struct Ellipse tEllipse;
        } data;
};

DATA struct Object *objects;
DATA int numObjects;

DATA float mousePosX;
DATA float mousePosY;
DATA float mouseStartX;
DATA float mouseStartY;
DATA float objectStartX;
DATA float objectStartY;
DATA float objectStartRadius;
DATA float zoomFactor;
DATA float transMat[2][2];
DATA int isHoveringObject;
DATA int isDraggingObject;
DATA int activeObject;

void setup_shapesrender(void);
void draw_shapes(void);

void setup_shapes(void);
Object add_circle(float x, float y, float radius);
Object add_ellipse(Object centerCircle0, Object centerCircle1, float radius);
void update_shapes(struct Input input);
