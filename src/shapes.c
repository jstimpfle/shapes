#include <shapes/defs.h>
#include <shapes/gfxrender.h>
#include <shapes/logging.h>
#include <shapes/memoryalloc.h>
#include <shapes/window.h>
#include <shapes/shapes.h>
#include <string.h>
#include <math.h>

float distance2d(float x0, float y0, float x1, float y1)
{
        float dx = (x0 - x1);
        float dy = (y0 - y1);
        return sqrtf(dx*dx + dy*dy);
}

int test_circle_hit(const struct Circle *circle, float x, float y)
{
        float cx = zoomFactor * circle->centerX;
        float cy = zoomFactor * circle->centerY;
        float radius = zoomFactor * circle->radius;
        return distance2d(cx, cy, x, y) < radius;
}

int test_ellipse_hit(const struct Ellipse *ellipse, float x, float y)
{
        struct Circle *c0 = &objects[ellipse->centerCircle0].data.tCircle;
        struct Circle *c1 = &objects[ellipse->centerCircle1].data.tCircle;
        float x0 = zoomFactor * c0->centerX;
        float y0 = zoomFactor * c0->centerY;
        float x1 = zoomFactor * c1->centerX;
        float y1 = zoomFactor * c1->centerY;
        float d0 = distance2d(x0, y0, x, y);
        float d1 = distance2d(x1, y1, x, y);
        return d0 + d1 < zoomFactor * ellipse->radius;
}

int test_object_hit(Object obj, float x, float y)
{
        if (objects[obj].objectKind == OBJECT_CIRCLE)
                return test_circle_hit(&objects[obj].data.tCircle, x, y);
        else if (objects[obj].objectKind == OBJECT_ELLIPSE)
                return test_ellipse_hit(&objects[obj].data.tEllipse, x, y);
        else
                assert(0);
}

Object add_circle(float x, float y, float radius)
{
        int obj = numObjects++;
        REALLOC_MEMORY(&objects, numObjects);
        objects[obj].objectKind = OBJECT_CIRCLE;
        objects[obj].data.tCircle.centerX = x;
        objects[obj].data.tCircle.centerY = y;
        objects[obj].data.tCircle.radius = radius;
        return obj;
}

Object add_ellipse(Object centerCircle0, Object centerCircle1, float radius)
{
        int obj = numObjects++;
        REALLOC_MEMORY(&objects, numObjects);
        objects[obj].objectKind = OBJECT_ELLIPSE;
        objects[obj].data.tEllipse.centerCircle0 = centerCircle0;
        objects[obj].data.tEllipse.centerCircle1 = centerCircle1;
        objects[obj].data.tEllipse.radius = radius;
        return obj;
}

void update_shapes(struct Input input)
{
        if (input.inputKind == INPUT_CURSORMOVE) {
                int x = input.data.tCursormove.pixelX;
                int y = input.data.tCursormove.pixelY;
                // unproject mouse position. XXX do it based on transMat
                mousePosX = (float) x / windowWidthInPixels;
                mousePosY = (float) (windowHeightInPixels - y) / windowWidthInPixels;
                if (isDraggingObject) {
                        float mouseDiffX = (mousePosX - mouseStartX) / zoomFactor;
                        float mouseDiffY = (mousePosY - mouseStartY) / zoomFactor;
                        struct Object *obj = &objects[activeObject];
                        if (obj->objectKind == OBJECT_ELLIPSE) {
                                obj->data.tEllipse.radius = objectStartRadius + mousePosX - mouseStartX;
                        }
                        else if (obj->objectKind == OBJECT_CIRCLE) {
                                obj->data.tCircle.centerX = objectStartX + mouseDiffX;
                                obj->data.tCircle.centerY = objectStartY + mouseDiffY;
                        }
                }
                else {
                        isHoveringObject = 0;
                        for (Object i = 0; i < numObjects; i++) {
                                if (test_object_hit(i, mousePosX, mousePosY)) {
                                        isHoveringObject = 1;
                                        activeObject = i;
                                        break;
                                }
                        }
                }
        }
        else if (input.inputKind == INPUT_MOUSEBUTTON) {
                if (input.data.tMousebutton.mousebuttonKind == MOUSEBUTTON_1) {
                        if (input.data.tMousebutton.mousebuttonEventKind == MOUSEBUTTONEVENT_PRESS) {
                                if (isHoveringObject) {
                                        isDraggingObject = 1;
                                        mouseStartX = mousePosX;
                                        mouseStartY = mousePosY;
                                        if (objects[activeObject].objectKind == OBJECT_CIRCLE) {
                                                struct Circle *circle = &objects[activeObject].data.tCircle;
                                                objectStartX = circle->centerX;
                                                objectStartY = circle->centerY;
                                        }
                                        else if (objects[activeObject].objectKind == OBJECT_ELLIPSE) {
                                                struct Ellipse *ellipse = &objects[activeObject].data.tEllipse;
                                                objectStartRadius = ellipse->radius;
                                        }
                                }
                        }
                        else if (input.data.tMousebutton.mousebuttonEventKind == MOUSEBUTTONEVENT_RELEASE) {
                                isDraggingObject = 0;
                        }
                }
        }
        else if (input.inputKind == INPUT_SCROLL) {
                if (input.data.tScroll.scrollKind == SCROLL_UP) {
                        zoomFactor += 0.5f;
                        if (zoomFactor > 5.0f)
                                zoomFactor = 5.0f;
                }
                else if (input.data.tScroll.scrollKind == SCROLL_DOWN) {
                        zoomFactor -= 0.5f;
                        if (zoomFactor < 1.0f)
                                zoomFactor = 1.0f;
                }
        }
}

void setup_shapes(void)
{
        zoomFactor = 1.0f;
}
