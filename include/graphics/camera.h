#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace graphics {

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class camera
{
public:
    enum class Movement {
        FORWARD_BACK,
        UP_DOWN,
        LEFT_RIGHT,
    };

    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Euler Angles
    float Yaw;
    float Pitch;
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    float deltaTime;

    // Constructor with vectors
    camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // Constructor with scalar values
    camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    inline void beginFrame (const float dt) { deltaTime = dt; }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 view()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void move (Movement direction, float amount)
    {
        float velocity = amount * deltaTime;
        switch (direction) {
            case Movement::FORWARD_BACK:
                Position -= Front * velocity;
                break;
            case Movement::LEFT_RIGHT: 
                Position += Right * velocity;
                break;
            case Movement::UP_DOWN: 
                Position += Up * velocity;
                break;
            default:
                break;
        };
    }

    void pan (const glm::vec3& direction) {
        Position += direction * deltaTime;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void orient (float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void zoom (float yoffset)
    {
        if (Zoom >= 1.0f && Zoom <= 45.0f)
            Zoom -= yoffset;
        if (Zoom <= 1.0f)
            Zoom = 1.0f;
        if (Zoom >= 45.0f)
            Zoom = 45.0f;
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

// class camera
// {
// public:
//     camera (float x, float y, float z) : position(x, y, z), angle(0) {
//         update(); // Generate front and up vectors
//     }
//     inline void beginFrame (const float dt) { deltaTime = dt; }
//     inline void panX (const float x) { position.x += x * deltaTime; }
//     inline void panY (const float y) { position.y += y * deltaTime; }
//     inline void panZ (const float z) { position.z += z * deltaTime; }
//     inline void pan (const float x, const float y, const float z) {
//         panX(x);
//         panY(y);
//         panZ(z);
//     }
//     inline void tilt (const float degrees) {
//         angle = degrees;
//         update();
//     }
//     inline void tiltBy (const float degrees) {
//         tilt(angle + (degrees * deltaTime));
//     }
//     inline glm::mat4 view () {
//         return glm::lookAt(position, position + front, up);
//     }

// private:
//     glm::vec3 position;
//     glm::vec3 front;
//     glm::vec3 up;
//     float deltaTime;
//     float angle;

//     inline void update () {
//         front = glm::normalize(glm::rotate(glm::vec3(0, 0, -1), glm::radians(angle), glm::vec3(1, 0, 0)));
//         auto right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 1.0f)));
//         up = glm::normalize(glm::cross(right, front));
//     }
// };

}

#endif // CAMERA_H