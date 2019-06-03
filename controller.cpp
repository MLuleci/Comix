#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>
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

Controller::Controller(char **argv)
{
	// Set-up defaults
	m_res_path = fs::path(argv[0]).parent_path().append("/res/");
	m_run = false;
	m_window = NULL;
	m_img = NULL;
	m_next = SDL_SCANCODE_RIGHT;
	m_prev = SDL_SCANCODE_LEFT;
	m_win_w = 640;
	m_win_h = 480;
	m_keep_zoom = false;
	m_font_size = 12;
	m_scroll = 0.02f;
	m_scale = 1.f;
	m_bar = {0};
	int flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;
	int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;

	// Check for res directory
	if (!fs::exists(m_res_path) && !fs::is_directory(m_res_path)) {
		std::cerr << "Cannot find 'res/' directory." << std::endl;
		return;
	}

	// Read & parse config file
	std::ifstream file_in(m_res_path.string() + ".config");
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
					int r, g, b;
					if (sscanf(&val[0], "(%d, %d, %d)", &r, &g, &b) != 3) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					} else {
						m_color[0] = (0 <= r && r <= 0xFF ? r : 0xFF);
						m_color[1] = (0 <= g && g <= 0xFF ? g : 0xFF);
						m_color[2] = (0 <= b && b <= 0xFF ? b : 0xFF);
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
					int _x, _y, _w, _h;
					if (sscanf(&val[0], "(%d, %d, %d, %d)", &_x, &_y, &_w, &_h) != 4) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					} else {
						x = _x;
						y = _y;
						m_win_w = (0 <= _w ? _w : m_win_w);
						m_win_h = (0 <= _h ? _h : m_win_h);
					}
				} else if (!opt.compare("borderless")) {
					int b = ParseBool(val);
					if (b == 1) {
						flags |= SDL_WINDOW_BORDERLESS;
					} else if (b != 0) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					}
				} else if (!opt.compare("fontsize")) {
					int tmp;
					if (sscanf(&val[0], "%d", &tmp) != 1 || tmp < 0) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					} else {
						m_font_size = tmp;
					}
				} else if (!opt.compare("scroll")) {
					float tmp; 
					if (sscanf(&val[0], "%f%%", &tmp) != 1 || tmp <= 0.f || tmp > 100.f) {
						std::cerr << "Value '" << val << "' invalid for option '" << opt << "'" << std::endl;
					} else {
						m_scroll = tmp / 100.f;
					}
				} else {
					std::cerr << "Option '" << opt << "' is not valid" << std::endl;
				}
			}
		}
		file_in.close();
	} else {
		std::cerr << "Failed to open config file. Using defaults..." << std::endl;
	}

	// Try and set texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cerr << "Warning: Linear texture filtering not enabled." << std::endl;
	}

	// Create m_window
	m_window = SDL_CreateWindow("ImageViewer", x, y, m_win_w, m_win_h, flags);
	if (m_window == NULL) {
		std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
		return;
	}

	// Create m_render
	m_render = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
	if (m_render == NULL) {
		std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
		return;
	}
	SDL_SetRenderDrawColor(m_render, m_color[0], m_color[1], m_color[2], 0xFF);

	// Load font
	if (m_font_size > 0) {
		for (auto& font : fs::directory_iterator(m_res_path)) {
			if (!font.path().extension().compare(".ttf")) {
				m_font = TTF_OpenFont(font.path().string().c_str(), m_font_size);
				if (m_font == NULL) {
					std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
					return;
				} else {
					break;
				}
			}
		}
		if (m_font == NULL) {
			std::cerr << "No font file(s) found." << std::endl;
			return;
		}
	}

	// Set-up path & directories
	fs::path file(argv[1]);
	if (!fs::exists(file.parent_path())) {
		file = fs::path("./").append(file);
	}
	if (Validate(file)) {
		for (auto &p : fs::directory_iterator(file.parent_path())) { // Add all files to vector
			if (Validate(p.path())) {
				m_list.push_back(p.path());
				if (fs::equivalent(p.path(), file)) m_index = m_list.size() - 1;
			}
		}
		m_img = LoadImage(file);
	} else {
		std::cerr << "Path must point to an image: jpeg, jpg, or png." << std::endl;
		return;
	}

	// Set m_cursor
	m_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	SDL_SetCursor(m_cursor);

	// Finally set m_run
	m_run = true;
}

Controller::~Controller() 
{
	// Save config changes
	std::string conf = m_res_path.string() + ".config";
	std::string tmp_conf = m_res_path.string() + "tmp.config";
	std::ifstream file_in(conf);
	std::ofstream file_out(tmp_conf);
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
		fs::remove(conf);
		fs::rename(tmp_conf, conf);
	} else {
		std::cerr << "Failed to save changes to config file." << std::endl;
	}
	if (fs::exists(tmp_conf)) {
		if (file_out.is_open()) file_out.close();
		fs::remove(tmp_conf);
	}

	// Close font
	TTF_CloseFont(m_font);
	m_font = NULL;

	// Free resources
	SDL_FreeCursor(m_cursor);
	SDL_DestroyTexture(m_img);
	for (auto t : m_text) SDL_DestroyTexture(t);
	SDL_DestroyRenderer(m_render);
	SDL_DestroyWindow(m_window);

	// Close subsystems
	TTF_Quit();
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
			SDL_Scancode code = sym.scancode;

			// Image navigation
			if (m_scale > 1.f) {
				if (code == SDL_SCANCODE_UP) {
					Move(0, static_cast<int>(m_rect.h * m_scroll));
				} else if (code == SDL_SCANCODE_DOWN) {
					Move(0, static_cast<int>(m_rect.h * -m_scroll));
				} else if (code == SDL_SCANCODE_HOME) {
					m_rect.y = 0;
					m_update = true;
				} else if (code == SDL_SCANCODE_END) {
					m_rect.y = m_win_h - m_rect.h;
					m_update = true;
				} else if (code == SDL_SCANCODE_PAGEUP) {
					Move(0, static_cast<int>(m_rect.h * 0.1f));
				} else if (code == SDL_SCANCODE_PAGEDOWN) {
					Move(0, static_cast<int>(m_rect.h * -0.1f));
				}
			}

			if (code == m_prev) { // Load previous image
				if (m_index == 0) m_index = m_list.size();
				m_index--;
				m_img = LoadImage(m_list.at(m_index));
			} else if (code == m_next) { // Load next image
				if (++m_index == m_list.size()) m_index = 0;
				m_img = LoadImage(m_list.at(m_index));
			} else if (sym.mod & KMOD_CTRL) {
				if (code == SDL_SCANCODE_EQUALS) { // Ctrl +
					Zoom(m_scale + 0.1f);
					CenterImage();
				} else if (code == SDL_SCANCODE_MINUS) { // Ctrl -
					Zoom(m_scale - 0.1f);
					CenterImage();
				} else if (code == SDL_SCANCODE_0) { // Ctrl 0
					Zoom(1.f);
					CenterImage();
				} else if (code == SDL_SCANCODE_1) { // Ctrl 1
					float mod = 1.f;
					if (m_win_w < m_img_w || m_win_h < m_img_h) { // Window smaller than image, fit it in
						mod *= fmin((float)m_win_w / (float)m_img_w, (float)m_win_h / (float)m_img_h);
					}
					Zoom(1.f / mod);
					CenterImage();
				} else if (code == SDL_SCANCODE_W) { // Ctrl W
					m_run = false;
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
			UpdateBar();
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
				m_bx = m_sx;
				m_by = m_sy;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (event->button.button == SDL_BUTTON_LEFT) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				if (x == m_bx && y == m_by) {
					SDL_Event evt;
					evt.type = SDL_KEYDOWN;
					if (m_next == SDL_SCANCODE_RIGHT) {
						evt.key.keysym.scancode = (x >= m_win_w / 2 ? m_next : m_prev);
					} else {
						evt.key.keysym.scancode = (x >= m_win_w / 2 ? m_prev : m_next);
					}
					SDL_PushEvent(&evt);
				}
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

	// Draw image
	SDL_Rect tmp = m_rect;
	tmp.h -= m_bar.h;
	SDL_RenderCopy(m_render, m_img, NULL, &tmp);

	if (m_font_size > 0) {
		// Draw bar
		SDL_SetRenderDrawColor(m_render, 0x2D, 0x2D, 0x30, 0xFF);
		SDL_RenderFillRect(m_render, &m_bar);

		// Draw text
		SDL_RenderCopy(m_render, m_text[0], NULL, &m_text_rect[0]);
		SDL_RenderCopy(m_render, m_text[1], NULL, &m_text_rect[1]);
	}
	SDL_SetRenderDrawColor(m_render, m_color[0], m_color[1], m_color[2], 0xFF);
	SDL_RenderPresent(m_render);
}

SDL_Texture *Controller::LoadImage(fs::path path)
{
	if (m_img != NULL) (m_img);
	SDL_Surface *surface = IMG_Load(path.u8string().c_str()); // Load surface
	SDL_Texture *texture = SDL_CreateTextureFromSurface(m_render, surface); // Load texture

	if (texture == NULL || surface == NULL) {
		std::cerr << "Failed to load texture or surface: " << SDL_GetError() << std::endl;
	} else {
		// Update info bar
		SDL_DestroyTexture(m_text[0]);
		SDL_Color white = {0xFF, 0xFF, 0xFF, 0xFF};
		m_text[0] = LoadText(path.filename().u8string().c_str(), white, &m_text_rect[0]);

		// Reset values
		m_img_w = surface->w;
		m_img_h = surface->h;
		Zoom((m_keep_zoom ? m_scale : 1.f));
		CenterImage();
		if (m_keep_zoom && m_img != NULL) {
			if (m_scale > 1.f) m_rect.y = 0;
		}

		SDL_FreeSurface(surface);
	}

	return texture;
}

SDL_Texture *Controller::LoadText(const char *text, SDL_Color color, SDL_Rect *rect)
{
	if (m_font_size <= 0) return NULL;
	SDL_Surface *surface = TTF_RenderUTF8_Blended(m_font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(m_render, surface);
	if (texture == NULL || surface == NULL) {
		std::cerr << "Failed to load texture or surface: " << SDL_GetError() << std::endl;
	} else {
		if (rect) {
			rect->w = surface->w;
			rect->h = surface->h;
		}
		SDL_FreeSurface(surface);
	}
	
	return texture;
}

bool Controller::Validate(fs::path path) 
{
	if (!path.empty() && !fs::is_directory(path) && fs::is_regular_file(path)) {
		std::string ext = path.extension().string();
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

void Controller::UpdateBar() 
{
	if (m_font_size <= 0) return;

	// Update info bar
	m_bar.h = static_cast<int>(m_font_size * 1.5f);
	m_bar.w = m_win_w;
	m_bar.x = 0;
	m_bar.y = m_win_h - m_bar.h;

	// Update text
	int pad = m_font_size / 4;
	m_text_rect[0].y = m_win_h - m_bar.h + pad;
	m_text_rect[1].y = m_win_h - m_bar.h + pad;
	m_text_rect[0].x = pad;
	m_text_rect[1].x = m_win_w - m_text_rect[1].w - pad;
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

	// Update info bar
	SDL_DestroyTexture(m_text[1]);
	SDL_Color white = {0xFF, 0xFF, 0xFF, 0xFF};
	std::stringstream ss;
	ss << static_cast<int>(m_rect.h * 100 / m_img_h) << "% | " << m_index + 1 << '/' << m_list.size();
	m_text[1] = LoadText(ss.str().c_str(), white, &m_text_rect[1]);

	UpdateBar();
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