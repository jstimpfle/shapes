#include <shapes/defs.h>
#include <shapes/gfxrender.h>
#include <shapes/logging.h>
#include <shapes/window.h>
#include <shapes/shapes.h>

enum {
        PROGRAM_ELLIPSE,
        PROGRAM_CIRCLE,
        PROGRAM_TEST,
        NUM_PROGRAM_KINDS,
};

enum {
        SHADER_PROJECTIONS_VERT,
        SHADER_ELLIPSE_FRAG,
        SHADER_CIRCLE_FRAG,
        SHADER_TEST_VERT,
        SHADER_TEST_FRAG,
        NUM_SHADER_KINDS,
};

enum {
        UNIFORM_ELLIPSE_projMat,
        UNIFORM_ELLIPSE_p0,
        UNIFORM_ELLIPSE_p1,
        UNIFORM_ELLIPSE_radius,
        UNIFORM_ELLIPSE_color,
        UNIFORM_CIRCLE_projMat,
        UNIFORM_CIRCLE_centerPoint,
        UNIFORM_CIRCLE_radius,
        UNIFORM_CIRCLE_color,
        NUM_UNIFORM_KINDS,
};

enum {
        ATTRIBUTE_ELLIPSE_position,
        ATTRIBUTE_CIRCLE_position,
        ATTRIBUTE_TEST_position,
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

enum {
        STATE_NORMAL,
        STATE_HOVERING,
        STATE_DRAGGING,
        NUM_STATES,
};

static const float ellipseColors[NUM_STATES][3] = {
        { 0.15f, 0.3f, 0.4f },
        { 0.2f, 0.4f, 0.5f },
        { 0.3f, 0.4f, 0.5f },
};

static const float circleColors[NUM_STATES][3] = {
        { 0.8f, 0.2f, 0.1f },
        { 0.85f, 0.2f, 0.2f },
        { 0.9f, 0.3f, 0.2f },
};

static const struct ShaderInfo shaderInfo[NUM_SHADER_KINDS] = {
#ifdef __EMSCRIPTEN__
#define MAKE(shaderKind, shaderType, shaderSource) [shaderKind] = { shaderType, #shaderKind, "#version 300 es\n" "precision highp float;\n" shaderSource }
#else
#define MAKE(shaderKind, shaderType, shaderSource) [shaderKind] = { shaderType, #shaderKind, "#version 130\n" shaderSource }
#endif
        MAKE(SHADER_PROJECTIONS_VERT, SHADER_VERTEX,
                "uniform mat3 projMat;\n"
                "vec4 compute_screenpos(vec2 position)\n"
                "{\n"
                "    vec3 v = projMat * vec3(position, 1.0);\n"
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
                "uniform vec2 p0;\n"
                "uniform vec2 p1;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 positionF;\n"
                "out vec4 out_color;\n"
                "void main()\n"
                "{\n"
                "    float d0 = distance(p0.xy, positionF);\n"
                "    float d1 = distance(p1.xy, positionF);\n"
                "    float d = d0 + d1;\n"
                "    float rdx = fwidth(d);\n"
                "    if (d > radius)\n"
                "        discard;\n"
                "    float val = (d - (radius - rdx)) / rdx;\n"
                "    out_color = vec4(color, 1.0 - val);\n"
                "}\n"),
        MAKE(SHADER_CIRCLE_FRAG, SHADER_FRAGMENT,
                "uniform mat3 projMat;\n"
                "uniform vec2 centerPoint;\n"
                "uniform float radius;\n"
                "uniform vec3 color;\n"
                "in vec2 positionF;\n"
                "out vec4 out_color;\n"
                "float compute_specular_strength(vec3 lightPos, vec3 surfacePoint, vec3 normalizedSurfaceNormal, vec3 spectatorPosition) {\n"
                "    vec3 lightToSurface = surfacePoint - lightPos;\n"
                "    vec3 reflectVector = normalize(lightToSurface - 2.0 * dot(lightToSurface, normalizedSurfaceNormal) * normalizedSurfaceNormal);\n"
                "    vec3 spectateDirection = normalize(spectatorPosition - surfacePoint);\n"
                "    float strength = pow(clamp(dot(reflectVector, spectateDirection), 0.0, 1.0), 4.0);\n"
                "    return strength;\n"
                "}\n"
                "void main()\n"
                "{\n"
                "    float d = distance(positionF, centerPoint);\n"
                "    if (d > radius)\n"
                "        discard;\n"
                /* Find height h which is the y-component such that vec3(positionF, h) is on the surface of the circle ("ball"). */
                /* That means that h must be such that h^2 + d^2 = radius^2 */
                "    float h = sqrt(radius * radius - d * d);\n"
                "    vec3 surfacePoint = vec3(positionF, h);\n"
                "    vec3 centerToSurface = surfacePoint - vec3(centerPoint, 0.0);\n"
                "    vec3 lightPos = vec3(0.2, 0.5, 5.0*radius);\n"
                "    vec3 lightPos2 = vec3(1.0, 1.0, 1.0);\n"
                "    vec3 surfaceToLight = lightPos - vec3(positionF, h);\n"
                "    vec3 spectatorPosition = vec3(0.5, 0.5, 6.0);\n"  // center of screen

                "    float dotProduct = dot(normalize(surfaceToLight), normalize(centerToSurface));\n"
                "    float diffuseStrength = clamp(dotProduct, 0.0, 1.0) + 0.2;\n"

                "    vec3 surfaceNormal = normalize(centerToSurface);\n"
                "    float specularStrength = compute_specular_strength(lightPos, surfacePoint, surfaceNormal, spectatorPosition);\n"
                "    float specularStrength2 = compute_specular_strength(lightPos2, surfacePoint, surfaceNormal, spectatorPosition);\n"

                /* smooth shape at the edges */
                "    float rdx = fwidth(d);\n"
                "    float val = (d - (radius - rdx)) / rdx;\n"

                " vec3 specularLight = vec3(0.0, 1.0, 1.0);\n"
                " vec3 specularLight2 = vec3(0.3, 0.0, 0.6);\n"
                " vec3 specularColor = 0.5 * specularStrength * specularLight;\n"
                " vec3 specularColor2 = 0.5 * specularStrength2 * specularLight2;\n"
                "    float strength = 0.1 + 0.3 * diffuseStrength;\n"
                "    out_color = vec4(strength * color + (specularColor + specularColor2), 1.0 - val);\n"
                "}\n"),
        MAKE(SHADER_TEST_VERT, SHADER_VERTEX,
                "in vec2 position;\n"
                "out vec2 p;\n"
                "void main()\n"
                "{\n"
                "    p = position;\n"
                "    gl_Position = vec4(position, 0.0, 1.0);\n"
                "}\n"),
        MAKE(SHADER_TEST_FRAG, SHADER_FRAGMENT,
                "in vec2 p;\n"
                "out vec4 out_color;\n"
                "float square(float x) { return x * x; }\n"
                "void main()\n"
                "{\n"
                "    vec4 bgColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                "    vec4 fgColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                "    float thickness = 0.05;\n"
                "    float r = 0.2;\n"
                "    float cx = 0.0;\n"
                "    float cy = 0.0;\n"
                "    float w = 0.5;\n"
                "    float h = 0.3;\n"
                "    float dx = abs(cx - p.x);\n"
                "    float dy = abs(cy - p.y);\n"
                "    float rdx = fwidth(p.x);\n"
                "    if ((w - r <= dx && dx < w)\n"
                "        && (h - r <= dy && dy < h)) {\n"
                "        float d = distance(vec2(dx, dy), vec2(w - r, h - r));\n"
                "        if (d > r - thickness / 2.0) {\n"
                "            float val = (d - (r - rdx)) / rdx;\n"
                "            out_color = vec4(fgColor.rgb, 1.0 - val);\n"
                "        }\n"
                "        else {\n"
                "            float val = (d - (r - thickness)) / rdx;\n"
                "            out_color = vec4(fgColor.rgb, val);\n"
                "        }\n"
                "    }\n"
                "    else {\n"
                "        float d = max(dx - (w - thickness), dy - (h - thickness));\n"
                "        if (d > thickness / 2.0) {\n"
                "            float val = (d - (thickness - rdx)) / rdx;\n"
                "            out_color = vec4(fgColor.rgb, 1.0 - val);\n"
                "        }\n"
                "        else {\n"
                "            float val = d / rdx;\n"
                "            out_color = vec4(fgColor.rgb, val);\n"
                "        }\n"
                "    }\n"
                "}\n"),
#undef MAKE
};

static const struct LinkInfo linkInfo[] = {
        { PROGRAM_ELLIPSE, SHADER_PROJECTIONS_VERT },
        { PROGRAM_CIRCLE, SHADER_PROJECTIONS_VERT },
        { PROGRAM_ELLIPSE, SHADER_ELLIPSE_FRAG },
        { PROGRAM_CIRCLE, SHADER_CIRCLE_FRAG },
        { PROGRAM_TEST, SHADER_TEST_FRAG },
        { PROGRAM_TEST, SHADER_TEST_VERT },
};

static const struct UniformInfo uniformInfo[NUM_UNIFORM_KINDS] = {
#define MAKE(x, y, z) [y] = { x, z }
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_projMat, "projMat" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_p0, "p0" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_p1, "p1" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_radius, "radius" ),
        MAKE( PROGRAM_ELLIPSE, UNIFORM_ELLIPSE_color, "color" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_projMat, "projMat" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_centerPoint, "centerPoint" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_radius, "radius" ),
        MAKE( PROGRAM_CIRCLE, UNIFORM_CIRCLE_color, "color" ),
#undef MAKE
};

static const struct AttributeInfo attributeInfo[NUM_ATTRIBUTE_KINDS] = {
#define MAKE(x, y, z) [y] = { x, z }
        MAKE( PROGRAM_ELLIPSE, ATTRIBUTE_ELLIPSE_position, "position" ),
        MAKE( PROGRAM_CIRCLE, ATTRIBUTE_CIRCLE_position, "position" ),
        MAKE( PROGRAM_TEST, ATTRIBUTE_TEST_position, "position" ),
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

static int get_object_state(Object obj)
{
        if (isDraggingObject && activeObject == obj)
                return STATE_DRAGGING;
        else if (isHoveringObject && activeObject == obj)
                return STATE_HOVERING;
        else
                return STATE_NORMAL;
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
        set_attribute_pointer(gfxVaoOfProgram[PROGRAM_TEST], attributeLocation[ATTRIBUTE_TEST_position], gfxVBO, 2, sizeof(struct Vec2), 0);
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
        int stateKind = get_object_state(ellipse);
        const float *color = ellipseColors[stateKind];
        set_GfxVBO_data(gfxVBO, &screenVerts, sizeof screenVerts);
        set_program_uniform_mat3f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_projMat], &projMat[0][0]);
        set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p0], ellipseControlPoints[0].x, ellipseControlPoints[0].y);
        set_program_uniform_2f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_p1], ellipseControlPoints[1].x, ellipseControlPoints[1].y);
        set_program_uniform_1f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_radius], e->radius);
        set_program_uniform_3f(gfxProgram[PROGRAM_ELLIPSE], uniformLocation[UNIFORM_ELLIPSE_color], color[0], color[1], color[2]);
        render_with_GfxProgram(gfxProgram[PROGRAM_ELLIPSE], gfxVaoOfProgram[PROGRAM_ELLIPSE], 0, LENGTH(screenVerts));
}

static void draw_point(Object obj)
{
        struct Circle *circle = &objects[obj].data.tCircle;
        float x = circle->centerX;
        float y = circle->centerY;
        float radius = circle->radius;
        float xa = x - 2.f * radius;
        float xb = x + 2.f * radius;
        float ya = y - 2.f * radius;
        float yb = y + 2.f * radius;
        const struct Vec2 smallVerts[] = {
                { xa, ya }, { xa, yb }, { xb, yb },
                { xa, ya }, { xb, yb }, { xb, ya }
        };        
        int stateKind = get_object_state(obj);
        const float *color = circleColors[stateKind];
        set_GfxVBO_data(gfxVBO, &smallVerts, sizeof smallVerts);
        set_program_uniform_mat3f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_projMat], &projMat[0][0]);
        set_program_uniform_2f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_centerPoint], x, y);
        set_program_uniform_1f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_radius], radius);
        set_program_uniform_3f(gfxProgram[PROGRAM_CIRCLE], uniformLocation[UNIFORM_CIRCLE_color], color[0], color[1], color[2]);
        render_with_GfxProgram(gfxProgram[PROGRAM_CIRCLE], gfxVaoOfProgram[PROGRAM_CIRCLE], 0, LENGTH(screenVerts));
}

void draw_shapes(void)
{
        clear_current_buffer();
        float ratio = (float) windowWidthInPixels / windowHeightInPixels;
        projMat[0][0] = zoomFactor * 2.0f;
        projMat[0][1] = 0.0f;
        projMat[0][2] = -zoomFactor;
        projMat[1][0] = 0.0f;
        projMat[1][1] = zoomFactor * ratio * 2.0f;
        projMat[1][2] = -zoomFactor;
        projMat[2][0] = 0.0f;
        projMat[2][1] = 0.0f;
        projMat[2][2] = 1.0f;
        unprojMat[0][0] = 1.0f / (zoomFactor * 2.0f);
        unprojMat[0][1] = 0.0f;
        unprojMat[0][2] = 1.0f / 2.0f;
        unprojMat[1][0] = 0.0f;
        unprojMat[1][1] = 1.0f / (zoomFactor * ratio * 2.0f);
        unprojMat[1][2] = 1.0f / (ratio * 2.0f);
        unprojMat[2][0] = 0.0f;
        unprojMat[2][1] = 0.0f;
        unprojMat[2][2] = 1.0f;

        {
        // two triangles covering the whole screen
        static const struct Vec2 screenVerts[] = {
                { -1.0f, -1.0f },
                { -1.0f, 1.0f },
                { 1.0f, 1.0f },
                { -1.0f, -1.0f },
                { 1.0f, -1.0f },
                { 1.0f, 1.0f },
        };
        clear_current_buffer();
        set_GfxVBO_data(gfxVBO, &screenVerts, sizeof screenVerts);
        render_with_GfxProgram(gfxProgram[PROGRAM_TEST], gfxVaoOfProgram[PROGRAM_TEST], 0, LENGTH(screenVerts));
        }

        for (Object i = 0; i < numObjects; i++)
                if (objects[i].objectKind == OBJECT_ELLIPSE)
                        draw_ellipse(i);
        for (Object i = 0; i < numObjects; i++)
                if (objects[i].objectKind == OBJECT_CIRCLE)
                        draw_point(i);
}
