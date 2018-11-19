#ifndef SHADER_H
#define SHADER_H

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>
#include <vector>

namespace graphics {
    typedef GLint uniform_t;
    typedef GLuint buffer_t;

    struct uniform {
        GLint location;

        inline void set(float v) const {return glUniform1f(location, v);}
        inline void set(int v) const {return glUniform1i(location, v);}
        inline void set(std::size_t v) const {return glUniform1i(location, v);}
        inline void set(const glm::vec2& v) const {return glUniform2fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::vec3& v) const {return glUniform3fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::vec4& v) const {return glUniform4fv(location, 1, glm::value_ptr(v));}
        inline void set(const glm::mat2& v) const {return glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(v));}
        inline void set(const glm::mat3& v) const {return glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(v));}
        inline void set(const glm::mat4& v) const {return glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));}
    };

    struct shader {
        enum class types {
            Fragment,
            Vertex,
            Geometry,
            TessControl,
            TessEval,
        };

        void unload() const;
        void bindUnfiromBlock(const std::string& blockName, unsigned int bindingPoint) const;
        graphics::uniform uniform(const std::string& name) const;
        
        inline void use () const {
            glUseProgram(programID);
        }

        static graphics::shader load (const std::map<graphics::shader::types,std::string>& shaderFiles);

        GLuint programID;
        std::vector<GLuint> shaders;
    };
}

#endif // SHADER_H
