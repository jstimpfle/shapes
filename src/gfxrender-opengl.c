#include <shapes/shapes.h>
#include <shapes/logging.h>
#include <shapes/memoryalloc.h>
#include <shapes/window.h>
#include <shapes/gfxrender.h>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  // needed for OpenGL on windows
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stddef.h>
#include <stdint.h>

struct GfxVBOInfo {
        GLuint vboId;
};

struct GfxVAOInfo {
        GLuint vaoId;
};

struct GfxShaderInfo {
        GLuint shaderId;
        const char *shaderName;
};

struct GfxProgramInfo {
        GLuint programId;
        const char *programName;
};


struct OpenGLInitInfo {
        void(**funcptr)(void);
        const char *name;
};

/* Define function pointers for all OpenGL extensions that we want to load */
#define MAKE(tp, name) static tp name;
#include <shapes/opengl-extensions.inc>
#undef MAKE

/* Define initialization info for all OpenGL extensions that we want to load */
const struct OpenGLInitInfo openGLInitInfo[] = {
#define MAKE(tp, name)  { (void(**)(void)) &name, #name },
#include <shapes/opengl-extensions.inc>
#undef MAKE
};

static struct GfxVBOInfo *gfxVBOInfo;
static struct GfxVAOInfo *gfxVAOInfo;
static struct GfxShaderInfo *gfxShaderInfo;
static struct GfxProgramInfo *gfxProgramInfo;

static int numGfxVBOs;
static int numGfxVAOs;
static int numGfxShaders;
static int numGfxPrograms;

static void check_gl_errors(const char *filename, int line)
{
        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
                fatalf("In %s line %d: GL error %s\n", filename, line,
                        gluErrorString(err));
}

#define CHECK_GL_ERRORS() check_gl_errors(__FILE__, __LINE__)

GfxVBO create_GfxVBO(void)
{
        GLuint vboId;
        glGenBuffers(1, &vboId);
        GfxVBO gfxVBO = numGfxVBOs++;
        REALLOC_MEMORY(&gfxVBOInfo, numGfxVBOs);
        gfxVBOInfo[gfxVBO].vboId = vboId;
        return gfxVBO;
}

GfxVAO create_GfxVAO(void)
{
        GLuint vaoId;
        glGenVertexArrays(1, &vaoId);
        GfxVAO gfxVao = numGfxVAOs++;
        REALLOC_MEMORY(&gfxVAOInfo, numGfxVAOs);
        gfxVAOInfo[gfxVao].vaoId = vaoId;
        return gfxVao;
}

GfxShader create_GfxShader(int shaderKind, const char *shaderName)
{
        static int glShaderKind;
        if (shaderKind == SHADER_VERTEX)
                glShaderKind = GL_VERTEX_SHADER;
        else if (shaderKind == SHADER_FRAGMENT)
                glShaderKind = GL_FRAGMENT_SHADER;
        else
                fatalf("Invalid value!\n");
        GLuint shaderId = glCreateShader(glShaderKind);
        GfxShader gfxShader = numGfxShaders++;
        REALLOC_MEMORY(&gfxShaderInfo, numGfxShaders);
        gfxShaderInfo[gfxShader].shaderId = shaderId;
        gfxShaderInfo[gfxShader].shaderName = shaderName;
        return gfxShader;
}

GfxProgram create_GfxProgram(const char *programName)
{
        GLuint programId = glCreateProgram();
        GfxProgram gfxProgram = numGfxPrograms++;
        REALLOC_MEMORY(&gfxProgramInfo, numGfxPrograms);
        gfxProgramInfo[gfxProgram].programId = programId;
        gfxProgramInfo[gfxProgram].programName = programName;
        return gfxProgram;
}

void set_GfxVBO_data(GfxVBO gfxVBO, const void *data, uint64_t size)
{
        GLuint vboId = gfxVBOInfo[gfxVBO].vboId;
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
}

UniformLocation get_uniform_location(GfxProgram gfxProgram, const char *uniformName)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        UniformLocation uniformLocation = glGetUniformLocation(programId, uniformName);
        if (uniformLocation < 0)
                log_postf("Failed to query attribute location for %s", uniformName);
        return uniformLocation;
}

AttributeLocation get_attribute_location(GfxProgram gfxProgram, const char *attribName)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        AttributeLocation attribLocation = glGetAttribLocation(programId, attribName);
        if (attribLocation < 0)
                log_postf("Failed to query attribute location for %s", attribName);
        return attribLocation;
}

void set_program_uniform_1f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        glUseProgram(programId);
        glUniform1f(uniformLocation, x);
        glUseProgram(0);
        CHECK_GL_ERRORS();
}

void set_program_uniform_2f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x, float y)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        glUseProgram(programId);
        glUniform2f(uniformLocation, x, y);
        glUseProgram(0);
        CHECK_GL_ERRORS();
}

void set_program_uniform_3f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x, float y, float z)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        glUseProgram(programId);
        glUniform3f(uniformLocation, x, y, z);
        glUseProgram(0);
        CHECK_GL_ERRORS();
}

void set_program_uniform_mat2f(GfxProgram gfxProgram, UniformLocation uniformLocation, float *fourFloats)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        glUseProgram(programId);
        glUniformMatrix2fv(uniformLocation, 1, GL_FALSE, fourFloats);
        glUseProgram(0);
        CHECK_GL_ERRORS();
}

void set_attribute_pointer(GfxVAO gfxVAO, AttributeLocation attribLocation, GfxVBO gfxVBO, int numFloats, int stride, int offset)
{
        GLuint vaoId = gfxVAOInfo[gfxVAO].vaoId;
        GLuint vboId = gfxVBOInfo[gfxVBO].vboId;
        glBindVertexArray(vaoId);
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        glEnableVertexAttribArray(attribLocation);
        glVertexAttribPointer(attribLocation,
                numFloats, GL_FLOAT, /*XXX need support for other types*/
                GL_FALSE, stride, (char*)0 + offset);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        CHECK_GL_ERRORS();
}

void set_GfxShader_source(GfxShader gfxShader, const char *source)
{
        GLuint shaderId = gfxShaderInfo[gfxShader].shaderId;
        glShaderSource(shaderId, 1, &source, NULL);
        CHECK_GL_ERRORS();
}

void compile_GfxShader(GfxShader gfxShader)
{
        GLuint shaderId = gfxShaderInfo[gfxShader].shaderId;
        const char *shaderName = gfxShaderInfo[gfxShader].shaderName;
        glCompileShader(shaderId);
        GLint compileStatus;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus != GL_TRUE) {
                GLchar errorBuf[1024];
                GLsizei length;
                glGetShaderInfoLog(shaderId, sizeof errorBuf, &length, errorBuf);
                fatalf("ERROR: shader %s failed to compile: %s\n", shaderName, errorBuf);
        }
}

void add_GfxShader_to_GfxProgram(GfxShader gfxShader, GfxProgram gfxProgram)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        GLuint shaderId = gfxShaderInfo[gfxShader].shaderId;
        glAttachShader(programId, shaderId);
}

void link_GfxProgram(GfxProgram gfxProgram)
{
        GLuint programId = gfxProgramInfo[gfxProgram].programId;
        const char *programName = gfxProgramInfo[gfxProgram].programName;
        glLinkProgram(programId);
        GLint linkStatus;
        glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
                GLsizei length;
                GLchar errorBuf[128];
                glGetProgramInfoLog(programId, sizeof errorBuf, &length, errorBuf);
                fatalf("ERROR: Failed to link shader program %s: %s\n", programName, errorBuf);
        }
}

void clear_current_buffer(void)
{
        CHECK_GL_ERRORS();
        //XXX
        //glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, windowWidthInPixels, windowHeightInPixels);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_with_GfxProgram(GfxProgram gfxProgram, GfxVAO gfxVAO, int first, int count)
{
        glUseProgram(gfxProgramInfo[gfxProgram].programId);
        glBindVertexArray(gfxVAOInfo[gfxVAO].vaoId);
        glDrawArrays(GL_TRIANGLES, first, count);
        glBindVertexArray(0);
        glUseProgram(0);
        CHECK_GL_ERRORS();
}

void setup_gfx(void)
{
        {
                GLint ctx_glMajorVersion;
                GLint ctx_glMinorVersion;
                /* This interface only exists in OpenGL 3.0 and higher */
                glGetIntegerv(GL_MAJOR_VERSION, &ctx_glMajorVersion);
                glGetIntegerv(GL_MINOR_VERSION, &ctx_glMinorVersion);
                log_postf("OpenGL version: %d.%d\n",
                        (int)ctx_glMajorVersion,
                        (int)ctx_glMinorVersion);
        }

        /* Load OpenGL function pointers */
        for (int i = 0; i < LENGTH(openGLInitInfo); i++) {
                const char *name = openGLInitInfo[i].name;
                ANY_FUNCTION *funcptr = window_get_OpenGL_function_pointer(name);
                if (funcptr == NULL)
                        fatalf("OpenGL extension %s not found\n", name);
                *openGLInitInfo[i].funcptr = funcptr;
        }
}
