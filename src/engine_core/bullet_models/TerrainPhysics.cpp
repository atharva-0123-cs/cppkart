#include "TerrainPhysics.h"
#include "engine_core/singletons/PhysicsWorldSingleton.h"

TerrainPhysics::TerrainPhysics(int width, int length, unsigned short* heightData, btScalar minHeight, btScalar maxHeight) {
        // Define the scale - adjust as needed
        btVector3 scale(1.0f, 1.0f, 1.0f);

        // Create a heightfield terrain shape
        auto* terrainShape = new btHeightfieldTerrainShape(
            width, length, heightData, 1.0, minHeight, maxHeight, 1, PHY_SHORT, false);

        // Set the local scaling
        //terrainShape->setLocalScaling(scale);

        // Define the terrain's position - adjust the position as needed
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));

        // Create the terrain rigid body
        btScalar mass(0.0); // Zero mass means static
        btVector3 localInertia(0, 0, 0);
        auto* motionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, terrainShape, localInertia);
        
        terrainRigidBody = new btRigidBody(rbInfo);
}
