#pragma once
#include <filesystem>
#include <SDL.h>
#include "widget.h"

class Button final : public Widget {
	SDL_Texture* _texture;
	SDL_Rect _rect;
public:
	Button(const std::filesystem::path&);
	Button(Button&&);
	~Button();

	void set_state(const State) override;
	void draw() const override;
};