#ifndef SHADER_H
#define SHADER_H

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace graphics {
    typedef GLint uniform_t;
    typedef GLuint buffer_t;

    struct uniform {
        GLint location;

        inline void set(float v) {return glUniform1f(location, v);}
        inline void set(int v) {return glUniform1i(location, v);}
        inline void set(std::size_t v) {return glUniform1i(location, v);}
        inline void set(const glm::vec2& v) {return glUniform2fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::vec3& v) {return glUniform3fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::vec4& v) {return glUniform4fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::mat2& v) {return glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(v));}
        inline void set(const glm::mat3& v) {return glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(v));}
        inline void set(const glm::mat4& v) {return glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));}
    };

    struct shader {
        GLuint programID;
        GLuint vertexProgram;
        GLuint fragmentProgram;

        void unload() const;
        void bindUnfiromBlock(const std::string& blockName, unsigned int bindingPoint) const;
        graphics::uniform uniform(const std::string& name) const;
        inline void use () const {
            glUseProgram(programID);
        }

        static graphics::shader load (const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename);
    };
}

#endif // SHADER_H
