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
        return distance2d(circle->centerX, circle->centerY, x, y) < circle->radius;
}

int test_ellipse_hit(const struct Ellipse *ellipse, float x, float y)
{
        struct Circle *c0 = &objects[ellipse->centerCircle0].data.tCircle;
        struct Circle *c1 = &objects[ellipse->centerCircle1].data.tCircle;
        float d0 = distance2d(c0->centerX, c0->centerY, x, y);
        float d1 = distance2d(c1->centerX, c1->centerY, x, y);
        return d0 + d1 < ellipse->radius;
}

int test_object_hit(Object obj, float x, float y)
{
        if (objects[obj].objectKind == OBJECT_CIRCLE)
                return test_circle_hit(&objects[obj].data.tCircle, x, y);
        else if (objects[obj].objectKind == OBJECT_ELLIPSE)
                return test_ellipse_hit(&objects[obj].data.tEllipse, x, y);
        else
                UNREACHABLE();
        log_postf("ASDF\n");
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
                // first, calculate OpenGL window space coordinates: (-1,1) x (-1,1)
                mousePosX = (float)x / windowWidthInPixels;
                mousePosY = (float) (windowHeightInPixels - y) / windowHeightInPixels;
                mousePosX = (mousePosX * 2) - 1.0f;
                mousePosY = (mousePosY * 2) - 1.0f;
                // now, unproject mouse position to world space. (We don't do all multiplications here since we know that most values are 0).
                mousePosX = unprojMat[0][0] * mousePosX + unprojMat[0][2];
                mousePosY = unprojMat[1][1] * mousePosY + unprojMat[1][2];
                if (isDraggingObject) {
                        float mouseDiffX = (mousePosX - mouseStartX);
                        float mouseDiffY = (mousePosY - mouseStartY);
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
