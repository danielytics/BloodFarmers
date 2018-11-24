#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include "util/logging.h"


namespace math {

struct rect {
    // Coordinate system: top_left.x <= bottom_right.x, top_left.y >= bottom_right.y
    glm::vec2 top_left;
    glm::vec2 bottom_right;

    inline bool containes (const glm::vec2& point) const
    {
        return point.x >= top_left.x && point.x <= bottom_right.x &&
               point.y >= top_left.y && point.y <= bottom_right.y;
    }
};

struct box {
    // Coordinate system: top_left.x <= bottom_right.x, top_left.y >= bottom_right.y, top_left.z >= bottom_right.z
    glm::vec3 top_left;
    glm::vec3 bottom_right;
};

struct frustum {
    enum Plane {
        PLANE_FAR,
        PLANE_NEAR,
        PLANE_RIGHT,
        PLANE_LEFT,
        PLANE_TOP,
        PLANE_BOTTOM
    };

    std::array<glm::vec4, 6> planes;

    frustum(const glm::mat4& matrix) {
        planes[PLANE_LEFT] = glm::vec4(matrix[0][3] + matrix[0][0], matrix[1][3] + matrix[1][0], matrix[2][3] + matrix[2][0], matrix[3][3] + matrix[3][0]);
        planes[PLANE_RIGHT] = glm::vec4(matrix[0][3] - matrix[0][0], matrix[1][3] - matrix[1][0], matrix[2][3] - matrix[2][0], matrix[3][3] - matrix[3][0]);
        planes[PLANE_TOP] = glm::vec4(matrix[0][3] - matrix[0][1], matrix[1][3] - matrix[1][1], matrix[2][3] - matrix[2][1], matrix[3][3] - matrix[3][1]);
        planes[PLANE_BOTTOM] = glm::vec4(matrix[0][3] + matrix[0][1], matrix[1][3] + matrix[1][1], matrix[2][3] + matrix[2][1], matrix[3][3] + matrix[3][1]);
        planes[PLANE_NEAR] = glm::vec4(matrix[0][3] + matrix[0][2], matrix[1][3] + matrix[1][2], matrix[2][3] + matrix[2][2], matrix[3][3] + matrix[3][2]);
        planes[PLANE_FAR] = glm::vec4(matrix[0][3] - matrix[0][2], matrix[1][3] - matrix[1][2], matrix[2][3] - matrix[2][2], matrix[3][3] - matrix[3][2]);

        for( int i = 0; i < 6; i++ )
        {
                planes[i] = glm::normalize(planes[i]);
        }
    }

    bool sphereIntersection(const glm::vec3& pos, float radius) const
    {
        // Loop through each plane that comprises the frustum.
        for (int i = 0; i < 6; i++)
        {
            // Plane-sphere intersection test. If p*n + d + r < 0 then we're outside the plane.
            // http://youtu.be/4p-E_31XOPM
            if (glm::dot(pos, glm::vec3(planes[i])) + planes[i].w + radius <= 0)
                return false;
        }

        // If none of the planes had the entity lying on its "negative" side then it must be
        // on the "positive" side for all of them. Thus the entity is inside or touching the frustum.
        return true;
    }
};

}

#endif // TYPES_H