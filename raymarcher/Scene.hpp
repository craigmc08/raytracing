#ifndef SCENE_H_
#define SCENE_H_

#include <stdlib.h>

#include "Vector.hpp"
#include "Camera.hpp"
#include "SceneObject.hpp"

class Scene {
private:
    int width;
    int height;
    int samples;

    Camera* camera;

    Vector sunDirection;
    Vector sunColor;

    std::vector<SceneObject*> objects;
public:
    void setSize(int w, int h) {
        width = w;
        height = h;
    }
    void setSamples(int s) {
        samples = s;
    }

    void SetCamera(Camera* cam) {
        camera = cam;
        camera->setSceneSettings(width, height);
    }
    void SetSun(Vector direction, Vector color) {
        sunDirection = direction.normalized();
        sunColor = color;
    }
    void AddObject(SceneObject* object) {
        objects.push_back(object);
    }

    void RenderToFile(const char* filename) {
        // TODO
        // Should render and output to a file
    }
};

#endif // SCENE_H_