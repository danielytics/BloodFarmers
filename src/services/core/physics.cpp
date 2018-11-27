#include "physics/engine.h"

void physics::Engine::init ()
{
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    broadphase = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0 , -10 , 0));
}

void physics::Engine::term ()
{
    delete dynamicsWorld;
    delete solver;
    delete broadphase;
    delete dispatcher;
    delete collisionConfiguration;
}

void physics::Engine::addBody (const ecs::entity entity, const Body& body, const Shape& shape)
{
        btCollisionShape* body_shape = new btBoxShape(btVector3(btScalar(shape.halfExtents.x), btScalar(shape.halfExtents.y), btScalar(shape.halfExtents.z)));

        collisionShapes.push_back(body_shape);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(body.position.x, body.position.y, body.position.z));

        btScalar mass(body.mass);

        //rigidbody is dynamic if and only if mass is non zero, otherwise static
        bool isDynamic = (mass != 0.f);

        btVector3 local_inertia(0, 0, 0);
        if (isDynamic) {
            body_shape->calculateLocalInertia(mass, local_inertia);
        }

        //using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
        btDefaultMotionState* motion_state = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rigitbody_info(mass, motion_state, body_shape, local_inertia);
        rigitbody_info.m_restitution = body.restitution;
        rigitbody_info.m_friction = body.friction;
        btRigidBody* rigid_body = new btRigidBody(rigitbody_info);
        rigid_body->setLinearVelocity(btVector3(0, 0, -5.0f));

        //add the body to the dynamics world
        dynamicsWorld->addRigidBody(rigid_body);
        physics_bodies[entity] = rigid_body;
}

void physics::Engine::addBodies (const std::vector<ecs::entity>& entities, const std::vector<Body>& bodies, const std::vector<Shape>& shapes)
{

}

void physics::Engine::getBodyPosition (const ecs::entity entity, glm::vec3& position)
{
    auto it = physics_bodies.find(entity);
    if (it != physics_bodies.end()) {
        auto [key, physics_body] = *it;
        const auto& transform = physics_body->getWorldTransform();
        auto origin = transform.getOrigin();
        position = glm::vec3(origin.x(), origin.y(), origin.z());
    }
}

void physics::Engine::getBodyPositions (const std::vector<ecs::entity> entities, std::vector<glm::vec3>& positions)
{

}

void physics::Engine::removeBody (const ecs::entity entity)
{
    auto it = physics_bodies.find(entity);
    if (it != physics_bodies.end()) {
        dynamicsWorld->removeRigidBody(it->second);
        physics_bodies.erase(it);
    }
}

void physics::Engine::removeBodies (const std::vector<ecs::entity>& entities)
{

}
