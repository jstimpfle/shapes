#include <shapes/defs.h>
#include <shapes/gfxrender.h>
#include <shapes/logging.h>
#include <shapes/shapes.h>

enum {
        PROGRAM_ELLIPSE,
        PROGRAM_CIRCLE,
        NUM_PROGRAM_KINDS,
};

enum {
        SHADER_PROJECTIONS_VERT,
        SHADER_ELLIPSE_FRAG,
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
        MAKE(SHADER_PROJECTIONS_VERT, SHADER_VERTEX,
                "#version 130\n"
                "uniform mat3 transMat;\n"
                "vec4 compute_screenpos(vec2 position)\n"
                "{\n"
                "    vec3 v = transMat * vec3(position, 1.0);\n"
                "    return vec4(v.xy, 0.0, 1.0);\n"
                "}\n"
                "in vec2 position;\n"
                "out vec2 positionF;\n"
                "void main()\n"
                "{\n"
                "    positionF = position;\n"
                "    gl_Position = compute_screenpos(position);\n"
                "}\n"),
        MAKE(SHADER_ELLIPSE_FRAG, SHADER_FRAGMENT,
                "#version 130\n"
                "uniform vec2 p0;\n"
                "uniform vec2 p1;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 positionF;\n"
                "void main()\n"
                "{\n"
                "    float d0 = distance(p0.xy, positionF);\n"
                "    float d1 = distance(p1.xy, positionF);\n"
                "    float d = d0 + d1;\n"
                "    float rdx = fwidth(d);\n"
                "    if (d > radius + rdx)\n"
                "        discard;\n"
                "    float val = (d - (radius - rdx)) / rdx;\n"
                "    gl_FragColor = vec4(color, 1.0 - val);\n"
                "}\n"),
        MAKE(SHADER_CIRCLE_FRAG, SHADER_FRAGMENT,
                "#version 130\n"
                "uniform mat3 transMat;\n"
                "uniform vec2 centerPoint;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 positionF;\n"
                "void main()\n"
                "{\n"
                "    float d = distance(positionF, centerPoint);\n"
                "    float rdx = fwidth(d);\n"
                "    if (d > radius + rdx)\n"
                "        discard;\n"
                "    float val = (d - (radius - rdx)) / rdx;\n"
                "    gl_FragColor = vec4(color, 1.0 - val);\n"
                "}\n"),
#undef MAKE
};

static struct LinkInfo linkInfo[] = {
        { PROGRAM_ELLIPSE, SHADER_PROJECTIONS_VERT },
        { PROGRAM_CIRCLE, SHADER_PROJECTIONS_VERT },
        { PROGRAM_ELLIPSE, SHADER_ELLIPSE_FRAG },
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
        { 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
};

static GfxShader gfxShader[NUM_SHADER_KINDS];
static GfxProgram gfxProgram[NUM_PROGRAM_KINDS];
static UniformLocation uniformLocation[NUM_UNIFORM_KINDS];
static AttributeLocation attributeLocation[NUM_ATTRIBUTE_KINDS];
static GfxVAO gfxVaoOfProgram[NUM_PROGRAM_KINDS];
static GfxVBO gfxVBO;

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

static void draw_ellipse(Object ellipse)
{
        const struct Ellipse *e = &objects[ellipse].data.tEllipse;
        const struct Circle *c0 = &objects[e->centerCircle0].data.tCircle;
        const struct Circle *c1 = &objects[e->centerCircle1].data.tCircle;
        const struct Vec2 ellipseControlPoints[2] = {
                { c0->centerX, c0->centerY },
                { c1->centerX, c1->centerY },
        };

        struct Vec3 color;
        if (isDraggingObject && activeObject == ellipse)
                color = (struct Vec3) {0.4f, 0.4f, 0.5f};
        else if (isHoveringObject && activeObject == ellipse)
                color = (struct Vec3) { 0.5, 0.0f, 0.0f };
        else
                color = (struct Vec3) { 0.0f, 0.0f, 1.0f };

        set_GfxVBO_data(gfxVBO, &screenVerts, sizeof screenVerts);
        set_program_uniform_mat3f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_transMat], &transMat[0][0]);
        set_program_uniform_3f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_color], color.x, color.y, color.z);
        set_program_uniform_1f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_radius], e->radius);
        set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p0], ellipseControlPoints[0].x, ellipseControlPoints[0].y);
        set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p1], ellipseControlPoints[1].x, ellipseControlPoints[1].y);
        render_with_GfxProgram(gfxProgram[PROGRAM_ELLIPSE], gfxVaoOfProgram[PROGRAM_ELLIPSE], 0, LENGTH(screenVerts));
}

static void draw_point(Object obj)
{
        struct Circle *circle = &objects[obj].data.tCircle;
        float x = circle->centerX;
        float y = circle->centerY;
        float radius = circle->radius;
        struct Vec3 color;
        float xa = x - 2.f * radius;
        float xb = x + 2.f * radius;
        float ya = y - 2.f * radius;
        float yb = y + 2.f * radius;
        const struct Vec2 smallVerts[] = {
                { xa, ya }, { xa, yb }, { xb, yb },
                { xa, ya }, { xb, yb }, { xb, ya }
        };
        if (isDraggingObject && activeObject == obj)
                color = (struct Vec3) { 0.4f, 0.2f, 0.8f };
        else if (isHoveringObject && activeObject == obj)
                color = (struct Vec3) { 0.1f, 0.1f, 0.4f };
        else
                color = (struct Vec3) { 0.8f, 0.8f, 0.8f };
        set_GfxVBO_data(gfxVBO, &smallVerts, sizeof smallVerts);
        set_program_uniform_mat3f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_transMat], &transMat[0][0]);
        set_program_uniform_2f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_centerPoint], x, y);
        set_program_uniform_1f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_radius], radius);
        set_program_uniform_3f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_color], color.x, color.y, color.z);
        render_with_GfxProgram(gfxProgram[PROGRAM_CIRCLE], gfxVaoOfProgram[PROGRAM_CIRCLE], 0, LENGTH(screenVerts));
}

void draw_shapes(void)
{
        clear_current_buffer();
        transMat[0][0] = zoomFactor * 2.0f;
        transMat[0][1] = 0.0f;
        transMat[0][2] = -zoomFactor;
        transMat[1][0] = 0.0f;
        transMat[1][1] = zoomFactor * 2.0f;
        transMat[1][2] = -zoomFactor;
        transMat[2][0] = 0.0f;
        transMat[2][1] = 0.0f;
        transMat[2][2] = 1.0f;
        for (Object i = 0; i < numObjects; i++)
                if (objects[i].objectKind == OBJECT_ELLIPSE)
                        draw_ellipse(i);
        for (Object i = 0; i < numObjects; i++)
                if (objects[i].objectKind == OBJECT_CIRCLE)
                        draw_point(i);
}
