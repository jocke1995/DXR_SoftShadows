#ifndef TESTSCENES_H
#define TESTSCENES_H

class SceneManager;
class Scene;

void EmptyUpdateScene(SceneManager* sm, double dt);

Scene* SponzaScene1(SceneManager* sm);
Scene* SponzaScene2(SceneManager* sm);
Scene* SponzaScene3(SceneManager* sm);
Scene* SponzaScene4(SceneManager* sm);

#endif