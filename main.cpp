#include <SDL3/SDL.h>
#include <iostream>
#include <string>

#include "Font.h"

enum DrawMethod {
    POINTS,
    LINES,
    CONTOURS,
};

// --------------------------------------------------------------------------

// Linearly remap an input x in [a, b] to [u, v].
float linear_remap(float x, float a, float b, float u, float v) {
    return (v - u) / (b - a) * (x - a) + u;
}

// --------------------------------------------------------------------------

void fit_rect_inside_another_rect(const SDL_FRect& inner_rect, const SDL_FRect& outer_rect, SDL_FRect& fitted_rect) {
    float inner_width_to_height_ratio = inner_rect.w / inner_rect.h;
    float inner_width_when_inner_height_is_maximized = inner_width_to_height_ratio * outer_rect.h;
    if (inner_width_when_inner_height_is_maximized <= outer_rect.w) {
        fitted_rect.w = inner_width_when_inner_height_is_maximized;
        fitted_rect.h = outer_rect.h;

        fitted_rect.x = outer_rect.x + (outer_rect.w - fitted_rect.w) / 2.0f;
        fitted_rect.y = outer_rect.y;
    } else {
        fitted_rect.w = outer_rect.w;
        fitted_rect.h = outer_rect.w / inner_width_to_height_ratio;

        fitted_rect.x = outer_rect.x;
        fitted_rect.y = outer_rect.y + (outer_rect.h - fitted_rect.h) / 2.0f;
    }
}

// --------------------------------------------------------------------------

void calculate_glyph_render_bounds(const Glyph& glyph, int window_width, int window_height, int padding, SDL_FRect& glyph_render_bounds) {
    SDL_FRect window_rect;
    window_rect.x = padding;
    window_rect.y = padding;
    window_rect.w = window_width - 2 * padding + 1;
    window_rect.h = window_height - 2 * padding + 1;

    SDL_FRect glyph_rect;
    glyph_rect.x = glyph.min_extents.x;
    glyph_rect.y = glyph.max_extents.y;
    glyph_rect.w = glyph.max_extents.x - glyph.min_extents.x + 1;
    glyph_rect.h = glyph.max_extents.y - glyph.min_extents.y + 1;

    fit_rect_inside_another_rect(glyph_rect, window_rect, glyph_render_bounds);
}

// --------------------------------------------------------------------------

void draw_glyph_points(SDL_Renderer* renderer, const Glyph& glyph, int window_width, int window_height, int padding) {
    SDL_FRect glyph_render_bounds;
    calculate_glyph_render_bounds(glyph, window_width, window_height, padding, glyph_render_bounds);

    for (int i = 0; i < glyph.num_points; i++) {
        float mapped_x = linear_remap(
            glyph.points[i].x,
            glyph.min_extents.x,
            glyph.max_extents.x,
            glyph_render_bounds.x,
            glyph_render_bounds.x + glyph_render_bounds.w - 1
        );
        float mapped_y = linear_remap(
            glyph.points[i].y,
            glyph.max_extents.y,
            glyph.min_extents.y,
            glyph_render_bounds.y,
            glyph_render_bounds.y + glyph_render_bounds.h - 1
        );

        SDL_RenderPoint(renderer, mapped_x, mapped_y);
    }
}

// --------------------------------------------------------------------------

void draw_glyph_lines(SDL_Renderer* renderer, const Glyph& glyph, int window_width, int window_height, int padding) {
    SDL_FRect glyph_render_bounds;
    calculate_glyph_render_bounds(glyph, window_width, window_height, padding, glyph_render_bounds);

    int current_first_point_index = 0;
    for (int i = 1; i < glyph.num_points; i++) {
        float mapped_x1 = linear_remap(
            glyph.points[i - 1].x,
            glyph.min_extents.x,
            glyph.max_extents.x,
            glyph_render_bounds.x,
            glyph_render_bounds.x + glyph_render_bounds.w - 1
        );
        float mapped_y1 = linear_remap(
            glyph.points[i - 1].y,
            glyph.max_extents.y,
            glyph.min_extents.y,
            glyph_render_bounds.y,
            glyph_render_bounds.y + glyph_render_bounds.h - 1
        );

        float mapped_x2 = linear_remap(
            glyph.points[i].x,
            glyph.min_extents.x,
            glyph.max_extents.x,
            glyph_render_bounds.x,
            glyph_render_bounds.x + glyph_render_bounds.w - 1
        );
        float mapped_y2 = linear_remap(
            glyph.points[i].y,
            glyph.max_extents.y,
            glyph.min_extents.y,
            glyph_render_bounds.y,
            glyph_render_bounds.y + glyph_render_bounds.h - 1
        );

        SDL_RenderLine(renderer, mapped_x1, mapped_y1, mapped_x2, mapped_y2);

        bool is_last_point_in_current_contour = false;
        for (int j = 0; j < glyph.num_end_point_indices; j++) {
            if (glyph.end_point_indices[j] == i) {
                is_last_point_in_current_contour = true;
                break;
            }
        }

        if (is_last_point_in_current_contour) {
            mapped_x1 = linear_remap(
                glyph.points[current_first_point_index].x,
                glyph.min_extents.x,
                glyph.max_extents.x,
                glyph_render_bounds.x,
                glyph_render_bounds.x + glyph_render_bounds.w - 1
            );
            mapped_y1 = linear_remap(
                glyph.points[current_first_point_index].y,
                glyph.max_extents.y,
                glyph.min_extents.y,
                glyph_render_bounds.y,
                glyph_render_bounds.y + glyph_render_bounds.h - 1
            );

            SDL_RenderLine(renderer, mapped_x1, mapped_y1, mapped_x2, mapped_y2);

            current_first_point_index = i + 1;
            i = current_first_point_index;
        }
    }
}

// --------------------------------------------------------------------------

void draw_glyph_contours(SDL_Renderer* renderer, const Glyph& glyph, int window_width, int window_height, int padding) {
    //todo - for now, show a red X to indicate no implementation

    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderLine(renderer, 0, 0, window_width - 1, window_height - 1);
    SDL_RenderLine(renderer, 0, window_height - 1, window_width - 1, 0);

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// --------------------------------------------------------------------------

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

    int window_width = 500;
    int window_height = 500;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[ERROR] Could not initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("TrueType Font Viewer", window_width, window_height, SDL_WINDOW_RESIZABLE);
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

    DrawMethod draw_method = DrawMethod::POINTS;

    // --- main loop ---

    bool is_running = true;
    while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                is_running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    is_running = false;
                } else if (event.key.scancode == SDL_SCANCODE_1) {
                    draw_method = DrawMethod::POINTS;
                } else if (event.key.scancode == SDL_SCANCODE_2) {
                    draw_method = DrawMethod::LINES;
                } else if (event.key.scancode == SDL_SCANCODE_3) {
                    draw_method = DrawMethod::CONTOURS;
                }
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                window_width = event.window.data1;
                window_height = event.window.data2;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        if (draw_method == DrawMethod::POINTS) {
            draw_glyph_points(renderer, glyph_0, window_width, window_height, 20);
        } else if (draw_method == DrawMethod::LINES) {
            draw_glyph_lines(renderer, glyph_0, window_width, window_height, 20);
        } else if (draw_method == DrawMethod::CONTOURS) {
            draw_glyph_contours(renderer, glyph_0, window_width, window_height, 20);
        }

        SDL_RenderPresent(renderer);
    }

    // --- cleanup ---

    glyph_0.destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
