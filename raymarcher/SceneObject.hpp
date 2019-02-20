#ifndef SCENEOBJECT_H_
#define SCENEOBJECT_H_

#include "Vector.hpp"
#include "Material.hpp"

// Base SceneObject class. Never returns collisions
class SceneObject {
private:
    SolidMaterial* material;
public:
    SolidMaterial* GetMaterial() { return material; }
    float GetDistance(Vector position) { return 1e9; }
};

#endif // SCENEOBJECT_H_