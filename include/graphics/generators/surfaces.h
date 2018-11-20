#ifndef GEN_SURFACES_H
#define GEN_SURFACES_H

#include "graphics/mesh.h"
#include "graphics/imagesets.h"

namespace graphics {

class Surface {
public:
    Surface(graphics::mesh mesh, int texture_unit) :
        mesh(mesh),
        texture_unit(texture_unit)
    {}

    inline void draw (const graphics::uniform& u_tileset) const {
        u_tileset.set(texture_unit);
        mesh.bind();
        mesh.draw();
    }

    inline void unload () {
        mesh.unload();
    }

private:
    graphics::mesh mesh;
    int texture_unit;
};

namespace generators {

class SurfacesGen {
    struct TempSurface {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> textureCoordinates;
    };
public:
    SurfacesGen (const graphics::Imagesets& imagesets) : imagesets(imagesets) {

    }

    void newSurface (const entt::hashed_string& id, float num_rows) {
        imageset_idx = imagesets.get(id);
        row = num_rows;
    }

    template <typename T, typename VertexTransformFn>
    void addRow (const std::vector<T> cells, VertexTransformFn vertexTransform) {
        auto& surface = surface_map[imageset_idx];
        float col = 0;
        for (const auto& layer : cells) {
            surface.vertices.push_back(vertexTransform(glm::vec4{col,   row  , 0, 1}));
            surface.vertices.push_back(vertexTransform(glm::vec4{col,   row-1, 0, 1}));
            surface.vertices.push_back(vertexTransform(glm::vec4{col+1, row-1, 0, 1}));
            surface.vertices.push_back(vertexTransform(glm::vec4{col+1, row-1, 0, 1}));
            surface.vertices.push_back(vertexTransform(glm::vec4{col+1, row  , 0, 1}));
            surface.vertices.push_back(vertexTransform(glm::vec4{col,   row  , 0, 1}));

            surface.textureCoordinates.push_back(glm::vec3(0, 0, layer));
            surface.textureCoordinates.push_back(glm::vec3(0, 1, layer));
            surface.textureCoordinates.push_back(glm::vec3(1, 1, layer));
            surface.textureCoordinates.push_back(glm::vec3(1, 1, layer));
            surface.textureCoordinates.push_back(glm::vec3(1, 0, layer));
            surface.textureCoordinates.push_back(glm::vec3(0, 0, layer));

            ++col;
        }
        --row;
    }

    std::vector<Surface> complete () {
        std::vector<Surface> surfaces;
        for (const auto& entry : surface_map) {
            graphics::mesh mesh;
            mesh.bind();
            mesh.addBuffer(entry.second.vertices, true);
            mesh.addBuffer(entry.second.textureCoordinates);
            surfaces.push_back({mesh, entry.first});
        }
        info("Loaded {} combined surfaces", surfaces.size());
        return surfaces;
    }

private:
    const graphics::Imagesets& imagesets;
    std::map<int, TempSurface> surface_map;
    // Per-surface temporary data
    int imageset_idx;
    float row;
};

}
}

#endif // GEN_SURFACES_H