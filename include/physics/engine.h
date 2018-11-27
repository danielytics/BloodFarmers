#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include <map>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btBox2dShape.h>

#include <services/core/physics.h>

namespace physics {

class Engine : public services::Physics {
public:
    void init ();
    void term ();

    
    void stepSimulation (float delta_time) {
        dynamicsWorld->stepSimulation(delta_time);
    }

    void addBody (const ecs::entity entity, const Body& body, const Shape& shape);
    void addBodies (const std::vector<ecs::entity>& entities, const std::vector<Body>& bodies, const std::vector<Shape>& shapes);

    void getBodyPosition (const ecs::entity entity, glm::vec3& position);
    void getBodyPositions (const std::vector<ecs::entity> entities, std::vector<glm::vec3>& positions);

    void removeBody (const ecs::entity entity);
    void removeBodies (const std::vector<ecs::entity>& entities);
    
private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* broadphase;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    btAlignedObjectArray<btCollisionShape*> collisionShapes;
    std::map<ecs::entity, btRigidBody*> physics_bodies;
};

}

#endif // PHYSICS_ENGINE_H