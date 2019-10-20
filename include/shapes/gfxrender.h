enum {
        SHADER_VERTEX,
        SHADER_FRAGMENT,
};

typedef int GfxVBO;
typedef int GfxVAO;
typedef int GfxShader;
typedef int GfxProgram;
typedef int UniformLocation;
typedef int AttributeLocation;

struct Vec2 {
        float x;
        float y;
};

struct Vec3 {
        float x;
        float y;
        float z;
};

GfxVBO create_GfxVBO(void);
GfxVAO create_GfxVAO(void);
GfxShader create_GfxShader(int shaderKind, const char *shaderName);
GfxProgram create_GfxProgram(const char *programName);
void set_GfxVBO_data(GfxVBO gfxVBO, const void *data, uint64_t size);
UniformLocation get_uniform_location(GfxProgram gfxProgram, const char *uniformName);
AttributeLocation get_attribute_location(GfxProgram gfxProgram, const char *attribName);
void set_program_uniform_1f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x);
void set_program_uniform_2f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x, float y);
void set_program_uniform_3f(GfxProgram gfxProgram, UniformLocation uniformLocation, float x, float y, float z);
void set_program_uniform_mat2f(GfxProgram gfxProgram, UniformLocation uniformLocation, float *fourFloats);
void set_attribute_pointer(GfxVAO gfxVaoOfProgram, AttributeLocation attribLocation, GfxVBO gfxVBO, int numFloats, int stride, int offset);
void set_GfxShader_source(GfxShader gfxShader, const char *source);
void compile_GfxShader(GfxShader gfxShader);
void add_GfxShader_to_GfxProgram(GfxShader gfxShader, GfxProgram gfxProgram);
void link_GfxProgram(GfxProgram gfxProgram);
void clear_current_buffer(void);
void render_with_GfxProgram(GfxProgram gfxProgram, GfxVAO gfxVaoOfProgram, int first, int count);
void setup_gfx(void);
