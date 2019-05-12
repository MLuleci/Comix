#include <iostream>
#include <fstream>
#include <cctype>
#include <cmath>
#include <SDL_image.h>
#include "controller.h"

std::string tolower(std::string str)
{
	for (char &c : str) {
		c = tolower(c);
	}
	return str;
}

int ParseBool(std::string str) 
{
	str = tolower(str);
	return (!str.compare("true") ? 1 : (!str.compare("false") ? 0 : -1));
}

Controller::Controller(char *path) : m_run(true), m_window(NULL), m_img(NULL), m_scale(1.f)
{
	// Set-up defaults
	m_next = SDL_SCANCODE_RIGHT;
	m_prev = SDL_SCANCODE_LEFT;
	m_win_w = 640;
	m_win_h = 480;
	m_keep_zoom = false;
	int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
	int r = 255, g = 255, b = 255;
	int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;

	// Read & parse config file
	std::ifstream file_in(".config");
	std::string line;
	if (file_in) {
		while (std::getline(file_in, line)) {
			size_t pos;
			if ((pos = line.find(':')) && pos != std::string::npos) {
				std::string opt, val;
				opt = line.substr(0, pos);
				val = line.substr(pos + 1);
				opt = tolower(opt);
				if (!opt.compare("keepzoom")) {
					int b = ParseBool(val);
					if (b == 1) {
						m_keep_zoom = true;
					} else if (b != 0) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("readright")) {
					int b = ParseBool(val);
					if (b == 0) {
						m_next = SDL_SCANCODE_LEFT;
						m_prev = SDL_SCANCODE_RIGHT;
					} else if (b != 1) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("background")) {
					if (sscanf(&val[0], "(%d, %d, %d)", &r, &g, &b) != 3) {
						r = 255;
						g = 255;
						b = 255;
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("mode")) {
					val = tolower(val);
					if (!val.compare("full")) {
						flags |= SDL_WINDOW_FULLSCREEN;
					} else if (!val.compare("max")) {
						flags |= SDL_WINDOW_MAXIMIZED;
					} else {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("lastsize")) {
					if (sscanf(&val[0], "(%d, %d, %d, %d)", &x, &y, &m_win_w, &m_win_h) != 4) {
						x = SDL_WINDOWPOS_CENTERED;
						y = SDL_WINDOWPOS_CENTERED;
						m_win_w = 640;
						m_win_h = 480;
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("borderless")) {
					int b = ParseBool(val);
					if (b == 1) {
						flags |= SDL_WINDOW_BORDERLESS;
					} else if (b != 0) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else {
					std::cerr << "Option '" << opt << "' is not valid" << std::endl;
				}
			}
		}
		file_in.close();
	} else {
		std::cerr << "Couldn't open or find config file, using defaults..." << std::endl;
	}

	// Create m_window
	m_window = SDL_CreateWindow("ImageViewer", x, y, m_win_w, m_win_h, flags);
	if (m_window == NULL) {
		std::cerr << "Couldn't create window: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	// Create m_render
	m_render = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
	if (m_render == NULL) {
		std::cerr << "Couldn't create renderer: " << SDL_GetError() << std::endl;
		exit(-1);
	}
	SDL_SetRenderDrawColor(m_render, r, g, b, 0xFF);

	// Set-up path & directories
	fs::path file = fs::path(path);
	if (Validate(file)) {
		m_img = LoadImage(file);
		for (auto &p : fs::directory_iterator(file.parent_path())) { // Add all files to vector
			if (Validate(p.path())) {
				m_list.push_back(p.path());
				if (fs::equivalent(p.path(), file)) m_index = m_list.size() - 1;
			}
		}
	} else {
		std::cerr << "Path must point to an image (jpeg/jpg/png)" << std::endl;
		exit(-1);
	}

	// Set m_cursor
	m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	SDL_SetCursor(m_cursor);
}

Controller::~Controller() 
{
	// Save config changes
	std::ifstream file_in(".config");
	std::ofstream file_out("tmp.config");
	if (file_in && file_out) {
		std::string line;
		while (std::getline(file_in, line)) {
			if (tolower(line).find("lastsize") == std::string::npos) {
				line += '\n';
				file_out.write(line.c_str(), line.length());
			}
		}

		// Append LastSize
		int x, y;
		SDL_GetWindowPosition(m_window, &x, &y);
		GetWinDim();
		char tmp[80];
		sprintf(tmp, "LastSize:(%d, %d, %d, %d)\n", x, y, m_win_w, m_win_h);
		line = tmp;
		file_out.write(line.c_str(), line.length());

		// Close files & rename temp
		file_in.close();
		file_out.close();
		fs::remove(".config");
		fs::rename("tmp.config", ".config");
	} else {
		std::cerr << "Couldn't save changes to config file, exiting without it..." << std::endl;
	}

	// Clean-up
	m_run = false;
	SDL_FreeCursor(m_cursor);
	SDL_DestroyTexture(m_img);
	SDL_DestroyRenderer(m_render);
	SDL_DestroyWindow(m_window);
	IMG_Quit();
	SDL_Quit();
}

int Controller::Loop() 
{
	SDL_Event event;
	while (m_run && SDL_WaitEvent(&event)) {
		Event(&event);

		if (m_update) {
			Render();
			m_update = false;
		}
	}

	return 0;
}

void Controller::Event(SDL_Event *event) 
{
	switch (event->type) {
		case SDL_QUIT:
			m_run = false;
			break;
		case SDL_KEYDOWN:
		{
			SDL_Keysym sym = event->key.keysym;
			if (sym.scancode == m_next) { // Load previous image
				if (m_index == 0) m_index = m_list.size();
				m_index--;
				m_img = LoadImage(m_list.at(m_index));
			} else if (sym.scancode == m_prev) { // Load next image
				if (++m_index == m_list.size()) m_index = 0;
				m_img = LoadImage(m_list.at(m_index));
			} else if (sym.mod & KMOD_CTRL) {
				if (sym.scancode == SDL_SCANCODE_EQUALS) { // Ctrl +
					Zoom(m_scale + 0.1f);
					CenterImage();
				} else if (sym.scancode == SDL_SCANCODE_MINUS) { // Ctrl -
					Zoom(m_scale - 0.1f);
					CenterImage();
				} else if (sym.scancode == SDL_SCANCODE_0) { // Ctrl 0
					Zoom(1.f);
					CenterImage();
				}
			}
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			/* mx, my = mouse position
			 * w, h = old (pre-zoom) dimensions
			 * dx, dy = dist. from image corner to mouse
			*/
			int mx, my, w, h, dx, dy;
			SDL_GetMouseState(&mx, &my);
			w = m_rect.w;
			h = m_rect.h;
			dx = mx - m_rect.x;
			dy = my - m_rect.y;
			Zoom(m_scale + sign(event->wheel.y) * 0.1f);
			CenterImage();
			if (m_scale > 1.f) {
				int x = static_cast<int>(mx - ((float)dx / (float)w) * m_rect.w - m_rect.x);
				int y = static_cast<int>(my - ((float)dy / (float)h) * m_rect.h - m_rect.y);
				Move(x, y);
			}
			break;
		}
		case SDL_WINDOWEVENT:
		{
			int w = m_win_w;
			int h = m_win_h;
			GetWinDim();
			if (w != m_win_w || h != m_win_h) {
				if (m_scale > 1.f) {
					if (m_win_w > m_rect.w) {
						m_rect.x = (m_win_w - m_rect.w) / 2;
					} else if (w > m_rect.w && m_win_w < m_rect.w) {
						m_rect.x = 0;
					}
					if (m_win_h > m_rect.h) {
						m_rect.y = (m_win_h - m_rect.h) / 2;
					} else if (h > m_rect.h && m_win_h < m_rect.h) {
						m_rect.y = 0;
					}
					m_update = true;
				} else {
					Zoom(m_scale);
					CenterImage();
				}
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
			if (event->button.button == SDL_BUTTON_LEFT) {
				SDL_GetMouseState(&m_sx, &m_sy);
			}
			break;
		case SDL_MOUSEMOTION:
			if (event->motion.state & SDL_BUTTON_LMASK) {
				int mx, my;
				SDL_GetMouseState(&mx, &my);
				Move(mx - m_sx, my - m_sy);
				m_sx = mx;
				m_sy = my;
			}
			break;
	}
}

void Controller::Render() 
{
	SDL_RenderClear(m_render);
	SDL_RenderCopy(m_render, m_img, NULL, &m_rect);
	SDL_RenderPresent(m_render);
}

SDL_Texture *Controller::LoadImage(fs::path path)
{
	if (m_img != NULL) SDL_DestroyTexture(m_img);
	SDL_Surface *surface = IMG_Load(path.generic_string().c_str()); // Load surface
	SDL_Texture *texture = SDL_CreateTextureFromSurface(m_render, surface); // Load texture

	// Reset values
	if (surface) {
		m_img_w = surface->w;
		m_img_h = surface->h;
		Zoom((m_keep_zoom ? m_scale : 1.f));
		CenterImage();
		if (m_keep_zoom && m_img != NULL) {
			if (m_scale > 1.f) m_rect.y = 0;
		}
		SDL_FreeSurface(surface);
	}
	if (texture == NULL || surface == NULL) {
		std::cerr << "Couldn't load texture/surface: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	return texture;
}

bool Controller::Validate(fs::path path) 
{
	if (!path.empty() && !fs::is_directory(path) && fs::is_regular_file(path)) {
		std::string ext = path.extension().generic_string();
		if (!ext.empty()) {
			ext = ext.substr(1, ext.length());
			for (std::string e : m_ext)
				if (!ext.compare(e)) return true;
		}
	}
	return false;
}

void Controller::GetWinDim()
{
	SDL_GetWindowSize(m_window, &m_win_w, &m_win_h); // Update width & height
}

void Controller::CenterImage() 
{
	m_rect.x = (m_win_w - m_rect.w) / 2;
	m_rect.y = (m_win_h - m_rect.h) / 2;
}

void Controller::Zoom(float scale) 
{
	m_scale = scale;

	// Limit scale [1, 2]
	if (m_scale < 1.f) {
		m_scale = 1.f;
	} else if (m_scale > 2.f) {
		m_scale = 2.f;
	}

	// Resize image
	GetWinDim();
	float mod = m_scale;
	if (m_win_w < m_img_w || m_win_h < m_img_h) { // Window smaller than image, fit it in
		mod *= fmin((float)m_win_w / (float)m_img_w, (float)m_win_h / (float)m_img_h);
	}
	m_rect.w = static_cast<int>(m_img_w * mod);
	m_rect.h = static_cast<int>(m_img_h * mod);

	// Change cursor
	if (m_scale > 1.f) {
		SDL_FreeCursor(m_cursor);
		m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
		SDL_SetCursor(m_cursor);
	} else {
		SDL_FreeCursor(m_cursor);
		m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
		SDL_SetCursor(m_cursor);
	}
	m_update = true;
}

void Controller::Move(int dx, int dy) 
{
	if (m_win_h < m_rect.h) { // The image has overflowing parts to move
		if (m_rect.y + dy <= 0 && m_rect.y + m_rect.h + dy >= m_win_h) {
			m_rect.y += dy; // Can move w/o leaving edges
		} else {
			// Move to max in the direction of y
			if (dy > 0) {
				m_rect.y = 0;
			} else {
				m_rect.y = m_win_h - m_rect.h;
			}
		}
	}
	if (m_win_w < m_rect.w) {
		if (m_rect.x + dx <= 0 && m_rect.x + m_rect.w + dx >= m_win_w) {
			m_rect.x += dx;
		} else {
			if (dx > 0) {
				m_rect.x = 0;
			} else {
				m_rect.x = m_win_w - m_rect.w;
			}
		}
	}
	m_update = true;
}