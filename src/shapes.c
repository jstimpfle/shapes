#include <shapes/defs.h>
#include <shapes/gfxrender.h>
#include <shapes/logging.h>
#include <shapes/memoryalloc.h>
#include <shapes/window.h>
#include <shapes/shapes.h>
#include <string.h>
#include <math.h>

enum {
        PROGRAM_ELLIPSE,
        PROGRAM_CIRCLE,
        NUM_PROGRAM_KINDS,
};

enum {
        SHADER_ELLIPSE_VERT,
        SHADER_ELLIPSE_FRAG,
        SHADER_CIRCLE_VERT,
        SHADER_CIRCLE_FRAG,
        NUM_SHADER_KINDS,
};

enum {
        UNIFORM_ELLIPSE_transMat,
        UNIFORM_ELLIPSE_p0,
        UNIFORM_ELLIPSE_p1,
        UNIFORM_ELLIPSE_radius,
        UNIFORM_ELLIPSE_color,
        UNIFORM_CIRCLE_transMat,
        UNIFORM_CIRCLE_centerPoint,
        UNIFORM_CIRCLE_radius,
        UNIFORM_CIRCLE_color,
        NUM_UNIFORM_KINDS,
};

enum {
        ATTRIBUTE_ELLIPSE_position,
        ATTRIBUTE_CIRCLE_position,
        NUM_ATTRIBUTE_KINDS,
};

struct ShaderInfo {
        int shaderType;
        const char *shaderName;
        const char *shaderSource;
};

struct LinkInfo {
        int programKind;
        int shaderKind;
};

struct UniformInfo {
        int programKind;
        const char *uniformName;
};

struct AttributeInfo {
        int programKind;
        const char *attributeName;
};

static struct ShaderInfo shaderInfo[NUM_SHADER_KINDS] = {
#define MAKE(shaderKind, shaderType, shaderSource) [shaderKind] = { shaderType, #shaderKind, shaderSource }
        MAKE(SHADER_ELLIPSE_VERT, SHADER_VERTEX,
                "#version 130\n"
                "uniform mat2 transMat;\n"
                "in vec2 position;\n"
                "out vec2 fragPosF;\n"
                "void main()\n"
                "{\n"
                "       fragPosF = position;\n"
                "       gl_Position = vec4(transMat * position, 0.0, 1.0);\n"
                "}\n"),
        MAKE(SHADER_ELLIPSE_FRAG, SHADER_FRAGMENT,
                "#version 130\n"
                "uniform vec2 p0;\n"
                "uniform vec2 p1;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 fragPosF;\n"
                "void main()\n"
                "{\n"
                "    float d0 = distance(p0.xy, fragPosF);\n"
                "    float d1 = distance(p1.xy, fragPosF);\n"
                "    float d = d0 + d1;\n"
                "    float fragDist = fwidth(d);\n"
                "    float rdx = fragDist * 5.0f; /* about 5 pixels */\n"
                "    if (d > radius + rdx)\n"
                "        discard;\n"
                "    float val = smoothstep(radius - rdx, radius + rdx, d);\n"
                "    gl_FragColor = vec4(color, 1.0 - val);\n"
                "}\n"),
        MAKE(SHADER_CIRCLE_VERT, SHADER_VERTEX,
                "#version 130\n"
                "uniform mat2 transMat;\n"
                "in vec2 position;\n"
                "out vec2 positionF;\n"
                "void main()\n"
                "{\n"
                "    positionF = position;\n"
                "    gl_Position = vec4(transMat * position, 0.0, 1.0);\n"
                "}\n"),
        MAKE(SHADER_CIRCLE_FRAG, SHADER_FRAGMENT,
                "#version 130\n"
                "uniform mat2 transMat;\n"
                "uniform vec2 centerPoint;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 positionF;\n"
                "void main()\n"
                "{\n"
                "    float d = distance(positionF, centerPoint);\n"
                "    float fragDist = fwidth(d);\n"
                "    float rdx = fragDist / 2.0;\n"
                "    if (d > radius)\n"
                "        discard;\n"
                "    gl_FragColor = vec4(color, 1.0 - smoothstep(radius - rdx, radius + rdx, d));\n"
                "}\n"),
#undef MAKE
};

static struct LinkInfo linkInfo[] = {
        { PROGRAM_ELLIPSE, SHADER_ELLIPSE_VERT },
        { PROGRAM_ELLIPSE, SHADER_ELLIPSE_FRAG },
        { PROGRAM_CIRCLE, SHADER_CIRCLE_VERT },
        { PROGRAM_CIRCLE, SHADER_CIRCLE_FRAG },
};

static const struct UniformInfo uniformInfo[NUM_UNIFORM_KINDS] = {
#define MAKE(x, y, z) [y] = { x, z }
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_transMat, "transMat" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_p0, "p0" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_p1, "p1" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_radius, "radius" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_color, "color" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_transMat, "transMat" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_centerPoint, "centerPoint" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_radius, "radius" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_color, "color" ),
#undef MAKE
};

static const struct AttributeInfo attributeInfo[NUM_ATTRIBUTE_KINDS] = {
#define MAKE(x, y, z) [y] = { x, z }
        MAKE( PROGRAM_ELLIPSE, ATTRIBUTE_ELLIPSE_position, "position" ),
        MAKE( PROGRAM_CIRCLE, ATTRIBUTE_CIRCLE_position, "position" ),
#undef MAKE
};

// two triangles covering the whole screen
static const struct Vec2 screenVerts[] = {
        { -1.0f, -1.0f },
        { -1.0f, 1.0f },
        { 1.0f, 1.0f },
        { -1.0f, -1.0f },
        { 1.0f, -1.0f },
        { 1.0f, 1.0f },
};

static GfxShader gfxShader[NUM_SHADER_KINDS];
static GfxProgram gfxProgram[NUM_PROGRAM_KINDS];
static UniformLocation uniformLocation[NUM_UNIFORM_KINDS];
static AttributeLocation attributeLocation[NUM_ATTRIBUTE_KINDS];
static GfxVAO gfxVaoOfProgram[NUM_PROGRAM_KINDS];
static GfxVBO gfxVBO;

static float mousePosX;
static float mousePosY;
static float zoomFactor = 1.0f;
static float transMat[2][2];
static int isHoveringEllipse;
static int isDraggingEllipse;
static int activeEllipse;
static float dragDiffX;
static float dragDiffY;

float distance2d(float x0, float y0, float x1, float y1)
{
        float dx = (x0 - x1);
        float dy = (y0 - y1);
        return sqrtf(dx*dx + dy*dy);
}

int test_ellipse_hit(const struct Ellipse *ellipse, float x, float y)
{
        float x0 = zoomFactor * ellipse->centerX0;
        float y0 = zoomFactor * ellipse->centerY0;
        float x1 = zoomFactor * ellipse->centerX1;
        float y1 = zoomFactor * ellipse->centerY1;
        float d0 = distance2d(x0, y0, x, y);
        float d1 = distance2d(x1, y1, x, y);
        return d0 + d1 <= zoomFactor * ellipse->radius;
}

void add_ellipse(const struct Ellipse *ellipse)
{
        int e = numEllipses++;
        REALLOC_MEMORY(&ellipseShapes, numEllipses);
        memcpy(&ellipseShapes[e], ellipse, sizeof *ellipse);
}

void add_circle(const struct Circle *circle)
{
        int x = numCircles++;
        REALLOC_MEMORY(&circleShapes, numCircles);
        memcpy(&circleShapes[x], circle, sizeof *circle);
}

void setup_shapesrender(void)
{        
        for (int i = 0; i < NUM_SHADER_KINDS; i++)
                gfxShader[i] = create_GfxShader(shaderInfo[i].shaderType, shaderInfo[i].shaderName);
        for (int i = 0; i < NUM_PROGRAM_KINDS; i++)
                gfxProgram[i] = create_GfxProgram("test-program");
        for (int i = 0; i < LENGTH(linkInfo); i++)
                add_GfxShader_to_GfxProgram(gfxShader[linkInfo[i].shaderKind], gfxProgram[linkInfo[i].programKind]);
        for (int i = 0; i < NUM_SHADER_KINDS; i++)
                set_GfxShader_source(gfxShader[i], shaderInfo[i].shaderSource);
        for (int i = 0; i < NUM_SHADER_KINDS; i++)
                compile_GfxShader(gfxShader[i]);
        for (int i = 0; i < NUM_PROGRAM_KINDS; i++)
                link_GfxProgram(gfxProgram[i]);
        for (int i = 0; i < NUM_UNIFORM_KINDS; i++)
                uniformLocation[i] = get_uniform_location(gfxProgram[uniformInfo[i].programKind], uniformInfo[i].uniformName);
        for (int i = 0; i < NUM_ATTRIBUTE_KINDS; i++)
                attributeLocation[i] = get_attribute_location(gfxProgram[attributeInfo[i].programKind], attributeInfo[i].attributeName);
        for (int i = 0; i < NUM_PROGRAM_KINDS; i++)
                gfxVaoOfProgram[i] = create_GfxVAO();
        gfxVBO = create_GfxVBO();
        set_attribute_pointer(gfxVaoOfProgram[PROGRAM_ELLIPSE], attributeLocation[ATTRIBUTE_ELLIPSE_position], gfxVBO, 2, sizeof(struct Vec2), 0);
        set_attribute_pointer(gfxVaoOfProgram[PROGRAM_CIRCLE], attributeLocation[ATTRIBUTE_CIRCLE_position], gfxVBO, 2, sizeof(struct Vec2), 0);
}

void update_shapes(struct Input input)
{
        if (input.inputKind == INPUT_CURSORMOVE) {
                int x = input.data.tCursormove.pixelX;
                int y = input.data.tCursormove.pixelY;
                mousePosX = (2.0f * x / windowWidthInPixels) - 1.0f;
                mousePosY = (2.0f * y / windowHeightInPixels) - 1.0f;

                if (isDraggingEllipse) {
                        ellipseShapes[activeEllipse].centerX0 = mousePosX + dragDiffX;
                        ellipseShapes[activeEllipse].centerY0 = mousePosY + dragDiffY;
                }
                else {
                        isHoveringEllipse = 0;
                        for (int i = 0; i < numEllipses; i++) {
                                struct Ellipse *e = &ellipseShapes[i];
                                if (test_ellipse_hit(e, mousePosX, mousePosY)) {
                                        isHoveringEllipse = 1;
                                        activeEllipse = i;
                                }
                        }
                }
        }
        else if (input.inputKind == INPUT_MOUSEBUTTON) {
                if (input.data.tMousebutton.mousebuttonKind == MOUSEBUTTON_1) {
                        if (input.data.tMousebutton.mousebuttonEventKind == MOUSEBUTTONEVENT_PRESS) {
                                if (isHoveringEllipse) {
                                        isDraggingEllipse = 1;
                                        dragDiffX = ellipseShapes[activeEllipse].centerX0 - mousePosX;
                                        dragDiffY = ellipseShapes[activeEllipse].centerY0 - mousePosY;
                                }
                        }
                        else if (input.data.tMousebutton.mousebuttonEventKind == MOUSEBUTTONEVENT_RELEASE) {
                                isDraggingEllipse = 0;
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

static void draw_ellipse_control_point(struct Vec2 point)
{
        float radius = 0.05f;
        struct Vec3 whiteColor = { 1.0f, 1.0f, 1.0f };
        float xa = point.x - 1.5f * radius;
        float xb = point.x + 1.5f * radius;
        float ya = point.y - 1.5f * radius;
        float yb = point.y + 1.5f * radius;
        const struct Vec2 smallVerts[] = {{ xa, ya }, { xa, yb }, { xb, yb }, { xa, ya }, { xb, yb }, { xb, ya }};
        set_GfxVBO_data(gfxVBO, &smallVerts, sizeof smallVerts);
        set_program_uniform_mat2f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_transMat], &transMat[0][0]);
        set_program_uniform_2f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_centerPoint], point.x, point.y);
        set_program_uniform_1f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_radius], radius);
        set_program_uniform_3f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_color], whiteColor.x, whiteColor.y, whiteColor.z);
        render_with_GfxProgram(gfxProgram[PROGRAM_CIRCLE], gfxVaoOfProgram[PROGRAM_CIRCLE], 0, LENGTH(screenVerts));
}

void draw_shapes(void)
{
        clear_current_buffer();
        transMat[0][0] = zoomFactor * 1.0f;
        transMat[0][1] = 0.0f;
        transMat[1][0] = 0.0f;
        transMat[1][1] = zoomFactor * -1.0f;
        for (int i = 0; i < numEllipses; i++) {
                struct Ellipse *e = &ellipseShapes[i];
                const struct Vec2 ellipseControlPoints[2] = {
                        { e->centerX0, e->centerY0 },
                        { e->centerX1, e->centerY1 },
                };

                struct Vec3 color;
                if (isDraggingEllipse && activeEllipse == i)
                        color = (struct Vec3) {0.5f, 1.0f, 1.0f};
                else if (isHoveringEllipse && activeEllipse == i)
                        color = (struct Vec3) { 0.5, 0.0f, 0.0f };
                else
                        color = (struct Vec3) { 0.1f, 0.1f, 0.1f };
                
                set_GfxVBO_data(gfxVBO, &screenVerts, sizeof screenVerts);
                set_program_uniform_mat2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_transMat], &transMat[0][0]);
                set_program_uniform_3f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_color], color.x, color.y, color.z);
                set_program_uniform_1f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_radius], e->radius);
                set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p0], ellipseControlPoints[0].x, ellipseControlPoints[0].y);
                set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p1], ellipseControlPoints[1].x, ellipseControlPoints[1].y);
                render_with_GfxProgram(gfxProgram[PROGRAM_ELLIPSE], gfxVaoOfProgram[PROGRAM_ELLIPSE], 0, LENGTH(screenVerts));
                
                draw_ellipse_control_point(ellipseControlPoints[0]);
                draw_ellipse_control_point(ellipseControlPoints[1]);
        }
}
