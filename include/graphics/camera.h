#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

namespace graphics {

class camera
{
public:
    camera (float x, float y, float z) : position(x, y, z), angle(0) {
        update(); // Generate front and up vectors
    }
    inline void beginFrame (const float dt) { deltaTime = dt; }
    inline void panX (const float x) { position.x += x * deltaTime; }
    inline void panY (const float y) { position.y += y * deltaTime; }
    inline void panZ (const float z) { position.z += z * deltaTime; }
    inline void pan (const float x, const float y, const float z) {
        panX(x);
        panY(y);
        panZ(z);
    }
    inline void tilt (const float degrees) {
        angle = degrees;
        update();
    }
    inline void tiltBy (const float degrees) {
        tilt(angle + (degrees * deltaTime));
    }
    inline glm::mat4 view () {
        return glm::lookAt(position, position + front, up);
    }

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float deltaTime;
    float angle;

    inline void update () {
        front = glm::normalize(glm::rotate(glm::vec3(0, 0, -1), glm::radians(angle), glm::vec3(1, 0, 0)));
        auto right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 1.0f)));
        up = glm::normalize(glm::cross(right, front));
    }
};

}

#endif // CAMERA_H