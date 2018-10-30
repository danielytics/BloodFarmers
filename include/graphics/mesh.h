#ifndef MESH_H
#define MESH_H

/**
 * Define a mesh of vertices and their attributes, stored in VBO's and attached to a VAO
 * Meshes can be used for many purposes: tile maps, sprites
 */

#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES 1
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

#include "shader.h"
#include "debug.h"

namespace graphics {
    namespace detail {
        template <typename T>
        struct VBOComponents {
            enum {NumComponents = 0};
        };

        template <>
        struct VBOComponents<float> {
            enum {NumComponents = 1};
        };

        template <>
        struct VBOComponents<glm::vec2> {
            enum {NumComponents = 2};
        };

        template <>
        struct VBOComponents<glm::vec3> {
            enum {NumComponents = 3};
        };

        template <>
        struct VBOComponents<glm::vec4> {
            enum {NumComponents = 4};
        };
    }

    class mesh
    {
    public:
        mesh () {
            glGenVertexArrays(1, &vao);
        }
        ~mesh () {
            glDeleteVertexArrays(1, &vao);
            for (auto vbo : vbos) {
                glDeleteBuffers(1, &vbo);
            }
        }

        inline void bind() {
            glBindVertexArray(vao);
        }

        template <typename T>
        unsigned addBuffer (const std::vector<T>& data, bool vertices=false) {
            GLuint id = GLuint(vbos.size());
            buffer_t vbo;
            // Create and bind the new buffer
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            // Copy the vertex data to the buffer
            auto vertexData = reinterpret_cast<const float*>(data.data());
            glBufferData(GL_ARRAY_BUFFER, data.size() * detail::VBOComponents<T>::NumComponents * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
            // Specify that the buffer data is going into attribute index 0, and contains N floats per vertex
            glVertexAttribPointer(id, detail::VBOComponents<T>::NumComponents, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(id);
            if (vertices) {
                count = data.size();
            }
            vbos.push_back(vbo);
            return id;
        }

        template <typename T>
        void setBuffer (unsigned id, const std::vector<T>& data) {
            buffer_t vbo = vbos[id];
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            // Copy the vertex data to the buffer
            auto vertexData = reinterpret_cast<const float*>(data.data());
            glBufferData(GL_ARRAY_BUFFER, data.size() * detail::VBOComponents<T>::NumComponents * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
        }

        void set (unsigned id, bool enabled) {
            if (enabled) {
                glEnableVertexAttribArray(id);
            } else {
                glDisableVertexAttribArray(id);
            }
        }

        unsigned addIndexBuffer () {
            GLuint id = GLuint(vbos.size());
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            vbos.push_back(ibo);
            return id;
        }

        inline void draw () {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, count);
        }
        inline void draw (unsigned int instances) {
            glBindVertexArray(vao);
            checkErrors();
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, count, instances);
            checkErrors();
        }
        inline void drawIndexed (const std::vector<GLushort>& indices)
        {
            glBindVertexArray(vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            auto indexData = reinterpret_cast<const GLushort*>(indices.data());
            auto size = indices.size() * sizeof(GLushort);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_STREAM_DRAW); // Orphan old buffer
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indexData, GL_STREAM_DRAW); // Upload new buffer
            glDrawElements(GL_TRIANGLES, GLsizei(indices.size()), GL_UNSIGNED_SHORT, nullptr);
        }

    private:
        GLsizei count;
        buffer_t vao;
        buffer_t ibo;
        std::vector<buffer_t> vbos;
    };
}

#endif // MESH_H
