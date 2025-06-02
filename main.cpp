#include <SDL3/SDL.h>
#include <iostream>
#include <string>

#include "Font.h"

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TTF_FONT_FILE" << std::endl;
        return 1;
    }

    std::string font_file_name = argv[1];
    Font font(font_file_name);

    Glyph glyph_0 = font.get_glyph();
    std::cout << std::endl;
    std::cout << "Glyph 0 data:" << std::endl;
    std::cout << "Bounds: (" << glyph_0.min_extents.x << ", " << glyph_0.min_extents.y << ") => (";
    std::cout << glyph_0.max_extents.x << ", " << glyph_0.max_extents.y << ")" << std::endl;

    std::cout << "End point indices: [";
    for (int i = 0; i < glyph_0.num_end_point_indices; i++) {
        std::cout << glyph_0.end_point_indices[i] << ((i == glyph_0.num_end_point_indices - 1) ? "]" : ", ");
    }
    std::cout << std::endl;

    for (int i = 0; i < glyph_0.num_points; i++) {
        std::cout << "Point " << i << " is " << (glyph_0.points[i].is_on_curve ? "ON curve : " : "OFF curve: ");
        std::cout << "(" << glyph_0.points[i].x << ", " << glyph_0.points[i].y << ")" << std::endl;
    }

    // --- setup ---

    const int WINDOW_WIDTH = 500;
    const int WINDOW_HEIGHT = 500;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[ERROR] Could not initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("TrueType Font Viewer", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (window == nullptr) {
        std::cerr << "[ERROR] Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "[ERROR] Could not create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        return 1;
    }

    SDL_SetRenderVSync(renderer, 1);

    // --- main loop ---

    bool is_running = true;
    while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                is_running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
                is_running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        SDL_FRect TODO_glyph_bounds_rect;
        TODO_glyph_bounds_rect.x = 20;
        TODO_glyph_bounds_rect.y = 20;
        TODO_glyph_bounds_rect.w = WINDOW_WIDTH - 2 * 20;
        TODO_glyph_bounds_rect.h = WINDOW_HEIGHT - 2 * 20;
        SDL_RenderRect(renderer, &TODO_glyph_bounds_rect);

        SDL_RenderPresent(renderer);
    }

    // --- cleanup ---

    glyph_0.destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
