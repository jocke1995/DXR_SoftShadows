#ifndef TESTSCENES_H
#define TESTSCENES_H

class SceneManager;
class Scene;

void EmptyUpdateScene(SceneManager* sm, double dt);

// Sponza test scenes
Scene* SponzaScene1(SceneManager* sm);
Scene* SponzaScene2(SceneManager* sm);
Scene* SponzaScene3(SceneManager* sm);
Scene* SponzaScene4(SceneManager* sm);

// Sponza + dragons test scenes
Scene* SponzaDragonsScene1(SceneManager* sm);
Scene* SponzaDragonsScene2(SceneManager* sm);
Scene* SponzaDragonsScene3(SceneManager* sm);
Scene* SponzaDragonsScene4(SceneManager* sm);

#endif