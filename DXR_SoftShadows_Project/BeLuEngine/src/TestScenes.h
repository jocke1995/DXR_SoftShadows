#ifndef TESTSCENES_H
#define TESTSCENES_H

class SceneManager;
class Scene;

void EmptyUpdateScene(SceneManager* sm, double dt);

// Sponza test scenes
Scene* SponzaScene1Light(SceneManager* sm);
Scene* SponzaScene2Lights(SceneManager* sm);
Scene* SponzaScene4Lights(SceneManager* sm);
Scene* SponzaScene8Lights(SceneManager* sm);

// Dragon test scenes
Scene* DragonScene1Light(SceneManager* sm);
Scene* DragonScene2Lights(SceneManager* sm);
Scene* DragonScene4Lights(SceneManager* sm);
Scene* DragonScene8Lights(SceneManager* sm);

// Sponza + dragons test scenes
Scene* SponzaDragonsScene1Light(SceneManager* sm);
Scene* SponzaDragonsScene2Lights(SceneManager* sm);
Scene* SponzaDragonsScene4Lights(SceneManager* sm);
Scene* SponzaDragonsScene8Lights(SceneManager* sm);

#endif