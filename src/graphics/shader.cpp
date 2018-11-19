
#include "graphics/shader.h"
#include "util/logging.h"
#include "util/helpers.h"

#include <fstream>
#include <iostream>

GLuint compileAndAttach (GLuint shaderProgram, GLenum shaderType, const std::string& filename, const std::string& shaderSource)
{
    GLuint shader = glCreateShader(shaderType);

    // Compile the shader
    char* source = const_cast<char*>(shaderSource.c_str());
    int32_t size = int32_t(shaderSource.length());
    glShaderSource(shader, 1, &source, &size);
    glCompileShader(shader);

    // Check for compile errors
    int wasCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &wasCompiled);
    if (wasCompiled == 0)
    {
        // Find length of shader info log
        int maxLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // Get shader info log
        char* shaderInfoLog = new char[maxLength];
        glGetShaderInfoLog(shader, maxLength, &maxLength, shaderInfoLog );

        fatal("Failed to compile shader:{}\n{}", filename, shaderInfoLog);

        delete [] shaderInfoLog;

        // Signal error
        return GLuint(-1);
    }

    // Attach the compiled program
    glAttachShader(shaderProgram, shader);
    return shader;
}

graphics::shader graphics::shader::load (const std::map<graphics::shader::types,std::string>& shaderFiles)
{
    static std::map<graphics::shader::types,GLenum> shaderTypes = {
        {graphics::shader::types::Vertex, GL_VERTEX_SHADER},
        {graphics::shader::types::Fragment, GL_FRAGMENT_SHADER},
        {graphics::shader::types::Geometry, GL_GEOMETRY_SHADER},
        {graphics::shader::types::TessControl, GL_TESS_CONTROL_SHADER},
        {graphics::shader::types::TessEval, GL_TESS_EVALUATION_SHADER},
    };

    GLuint shaderProgram = glCreateProgram();
    std::vector<GLuint> shaders;
    for (auto [type, filename] : shaderFiles) {
        auto shaderType = shaderTypes[type];
        auto source = helpers::readToString(filename);
        GLuint shader = compileAndAttach(shaderProgram, shaderType, filename, source);
        shaders.push_back(shader);
    }
    // Link the shader programs into one
    glLinkProgram(shaderProgram);
    int isLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, reinterpret_cast<int*>(&isLinked));
    if (!isLinked) {
        // Find length of shader info log
        int maxLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

        // Get shader info log
        char* shaderProgramInfoLog = new char[maxLength];
        glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, shaderProgramInfoLog);

        delete [] shaderProgramInfoLog;
        fatal("Linking shaders failed.\n{}", shaderProgramInfoLog);
    }
    return {shaderProgram, shaders};
}


void graphics::shader::unload () const
{
    glUseProgram(0);
    for (auto shader : shaders) {
        glDetachShader(programID, shader);
        glDeleteShader(shader);
    }
    glDeleteProgram(programID);
}

void graphics::shader::bindUnfiromBlock(const std::string& blockName, unsigned int bindingPoint) const
{
    GLuint location = glGetUniformBlockIndex(programID, blockName.c_str());
    glUniformBlockBinding(programID, location, bindingPoint);
}

graphics::uniform graphics::shader::uniform(const std::string& name) const
{
    return {glGetUniformLocation(programID, name.c_str())};
}
