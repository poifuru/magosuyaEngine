#pragma once

namespace MyEngine::LowLevel {
	class Engine;
}

namespace MyEngine::Rendering {
	class Renderer;
}

class SceneManager;

class Game {
public:
	Game();
	~Game();

	void Run();

private:
	std::unique_ptr<MyEngine::LowLevel::Engine> engine_ = nullptr;
	std::unique_ptr<MyEngine::Rendering::Renderer> renderer_ = nullptr;
	std::unique_ptr<SceneManager> sceneManager_ = nullptr;
};