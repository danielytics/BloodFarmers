
#include "graphics/shader.h"
#include "util/logging.h"
#include "util/helpers.h"

#include <fstream>
#include <iostream>

GLuint compileAndAttach (GLuint shaderProgram, GLenum programType, const std::string& filename, const std::string& shaderSource)
{
    GLuint program = glCreateShader(programType);

    // Compile the shader
    char* source = const_cast<char*>(shaderSource.c_str());
    int32_t size = int32_t(shaderSource.length());
    glShaderSource(program, 1, &source, &size);
    glCompileShader(program);

    // Check for compile errors
    int wasCompiled = 0;
    glGetShaderiv(program, GL_COMPILE_STATUS, &wasCompiled);
    if (wasCompiled == 0)
    {
        // Find length of shader info log
        int maxLength;
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        // Get shader info log
        char* shaderInfoLog = new char[maxLength];
        glGetShaderInfoLog(program, maxLength, &maxLength, shaderInfoLog );

        fatal("Failed to compile shader:{}\n{}", filename, shaderInfoLog);

        delete [] shaderInfoLog;

        // Signal error
        return GLuint(-1);
    }

    // Attach the compiled program
    glAttachShader(shaderProgram, program);
    return program;
}

graphics::shader createShader (const std::string& vertexShaderFilename, const std::string& vertexShader, const std::string& fragmentShaderFilename, const std::string& fragmentShader)
{
    GLuint shaderProgram = glCreateProgram();

    // Compile shader programs
    GLuint vertexProgram = compileAndAttach(shaderProgram, GL_VERTEX_SHADER, vertexShaderFilename, vertexShader);
    GLuint fragmentProgram = compileAndAttach(shaderProgram, GL_FRAGMENT_SHADER, fragmentShaderFilename, fragmentShader);

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

        fatal("Linking shaders {} and {} failed.\n{}", vertexShaderFilename, fragmentShaderFilename, shaderProgramInfoLog);

        delete [] shaderProgramInfoLog;

        return {};
    }

    return {shaderProgram, vertexProgram, fragmentProgram};
}

graphics::shader graphics::shader::load (const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename) {
    return createShader(vertexShaderFilename, helpers::readToString(vertexShaderFilename),
                        fragmentShaderFilename, helpers::readToString(fragmentShaderFilename));
}


void graphics::shader::unload () const
{
    glUseProgram(0);
    glDetachShader(programID, vertexProgram);
    glDetachShader(programID, fragmentProgram);
    glDeleteProgram(programID);
    glDeleteShader(vertexProgram);
    glDeleteShader(fragmentProgram);
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
