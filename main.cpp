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
    Uint8 draw_r, draw_g, draw_b, draw_a;
    SDL_GetRenderDrawColor(renderer, &draw_r, &draw_g, &draw_b, &draw_a);

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

        if (!glyph.points[i].is_on_curve) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, draw_r, draw_g, draw_b, draw_a);
        }

        SDL_FRect point_rect;
        point_rect.x = mapped_x - 1;
        point_rect.y = mapped_y - 1;
        point_rect.w = 3;
        point_rect.h = 3;
        SDL_RenderRect(renderer, &point_rect);
    }

    SDL_SetRenderDrawColor(renderer, draw_r, draw_g, draw_b, draw_a);
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

int wrap(int value, int min, int max) {
    if (value < min) {
        int delta = value - min;
        return max + delta + 1;
    } else if (value > max) {
        int delta = value - max;
        return min + delta - 1;
    } else {
        return value;
    }
}

// --------------------------------------------------------------------------

void draw_direct_line(SDL_Renderer* renderer, const Glyph& glyph, const SDL_FRect& glyph_render_bounds, const GlyphPoint& p1, const GlyphPoint& p2) {
    float mapped_x1 = linear_remap(
        p1.x,
        glyph.min_extents.x,
        glyph.max_extents.x,
        glyph_render_bounds.x,
        glyph_render_bounds.x + glyph_render_bounds.w - 1
    );
    float mapped_y1 = linear_remap(
        p1.y,
        glyph.max_extents.y,
        glyph.min_extents.y,
        glyph_render_bounds.y,
        glyph_render_bounds.y + glyph_render_bounds.h - 1
    );

    float mapped_x2 = linear_remap(
        p2.x,
        glyph.min_extents.x,
        glyph.max_extents.x,
        glyph_render_bounds.x,
        glyph_render_bounds.x + glyph_render_bounds.w - 1
    );
    float mapped_y2 = linear_remap(
        p2.y,
        glyph.max_extents.y,
        glyph.min_extents.y,
        glyph_render_bounds.y,
        glyph_render_bounds.y + glyph_render_bounds.h - 1
    );

    SDL_RenderLine(renderer, mapped_x1, mapped_y1, mapped_x2, mapped_y2);
}

// --------------------------------------------------------------------------

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// --------------------------------------------------------------------------

void lerp_points(float x1, float y1, float x2, float y2, float t, SDL_FPoint& interpolated_point) {
    interpolated_point.x = lerp(x1, x2, t);
    interpolated_point.y = lerp(y1, y2, t);
}

// --------------------------------------------------------------------------

void draw_quadratic_bezier_curve(SDL_Renderer* renderer, const Glyph& glyph, const SDL_FRect& glyph_render_bounds, const GlyphPoint& p1, const GlyphPoint& p2, const GlyphPoint& p3, int subdivisions) {
    float mapped_x1 = linear_remap(
        p1.x,
        glyph.min_extents.x,
        glyph.max_extents.x,
        glyph_render_bounds.x,
        glyph_render_bounds.x + glyph_render_bounds.w - 1
    );
    float mapped_y1 = linear_remap(
        p1.y,
        glyph.max_extents.y,
        glyph.min_extents.y,
        glyph_render_bounds.y,
        glyph_render_bounds.y + glyph_render_bounds.h - 1
    );

    float mapped_x2 = linear_remap(
        p2.x,
        glyph.min_extents.x,
        glyph.max_extents.x,
        glyph_render_bounds.x,
        glyph_render_bounds.x + glyph_render_bounds.w - 1
    );
    float mapped_y2 = linear_remap(
        p2.y,
        glyph.max_extents.y,
        glyph.min_extents.y,
        glyph_render_bounds.y,
        glyph_render_bounds.y + glyph_render_bounds.h - 1
    );

    float mapped_x3 = linear_remap(
        p3.x,
        glyph.min_extents.x,
        glyph.max_extents.x,
        glyph_render_bounds.x,
        glyph_render_bounds.x + glyph_render_bounds.w - 1
    );
    float mapped_y3 = linear_remap(
        p3.y,
        glyph.max_extents.y,
        glyph.min_extents.y,
        glyph_render_bounds.y,
        glyph_render_bounds.y + glyph_render_bounds.h - 1
    );

    float increment = 1.0f / subdivisions;
    for (float t = 0.0f; t < 1.0f; t += increment) {
        SDL_FPoint p_1_to_2;
        SDL_FPoint p_2_to_3;

        lerp_points(mapped_x1, mapped_y1, mapped_x2, mapped_y2, t, p_1_to_2);
        lerp_points(mapped_x2, mapped_y2, mapped_x3, mapped_y3, t, p_2_to_3);

        SDL_FPoint current_curve_point;
        lerp_points(p_1_to_2.x, p_1_to_2.y, p_2_to_3.x, p_2_to_3.y, t, current_curve_point);

        float next_t = t + increment;
        if (next_t > 1.0f) {
            next_t = 1.0f;
        }

        lerp_points(mapped_x1, mapped_y1, mapped_x2, mapped_y2, next_t, p_1_to_2);
        lerp_points(mapped_x2, mapped_y2, mapped_x3, mapped_y3, next_t, p_2_to_3);

        SDL_FPoint next_curve_point;
        lerp_points(p_1_to_2.x, p_1_to_2.y, p_2_to_3.x, p_2_to_3.y, next_t, next_curve_point);

        SDL_RenderLine(renderer, current_curve_point.x, current_curve_point.y, next_curve_point.x, next_curve_point.y);
    }
}

// --------------------------------------------------------------------------

void draw_glyph_contours(SDL_Renderer* renderer, const Glyph& glyph, int window_width, int window_height, int padding, int subdivisions) {
    SDL_FRect glyph_render_bounds;
    calculate_glyph_render_bounds(glyph, window_width, window_height, padding, glyph_render_bounds);

    int lower_index = 0;
    for (int endpoint_index = 0; endpoint_index < glyph.num_end_point_indices; endpoint_index++) {
        int upper_index = glyph.end_point_indices[endpoint_index];

        for (int i = lower_index; i <= upper_index; i++) {
            int first_index = -1;
            int second_index = -1;
            int third_index = -1;

            GlyphPoint current_point = glyph.points[i];
            if (current_point.is_on_curve) {
                first_index = i;
                second_index = wrap(i + 1, lower_index, upper_index);
                third_index = wrap(i + 2, lower_index, upper_index);
            } else {
                first_index = wrap(i - 1, lower_index, upper_index);
                second_index = i;
                third_index = wrap(i + 1, lower_index, upper_index);
            }

            GlyphPoint first_point = glyph.points[first_index];
            GlyphPoint second_point = glyph.points[second_index];
            GlyphPoint third_point = glyph.points[third_index];

            if (current_point.is_on_curve) {
                if (second_point.is_on_curve) {
                    draw_direct_line(renderer, glyph, glyph_render_bounds, first_point, second_point);
                } else {
                    if (third_point.is_on_curve) {
                        draw_quadratic_bezier_curve(renderer, glyph, glyph_render_bounds, first_point, second_point, third_point, subdivisions);
                    } else {
                        GlyphPoint mid_point;
                        mid_point.is_on_curve = true;
                        mid_point.x = (second_point.x + third_point.x) / 2.0f;
                        mid_point.y = (second_point.y + third_point.y) / 2.0f;

                        draw_quadratic_bezier_curve(renderer, glyph, glyph_render_bounds, first_point, second_point, mid_point, subdivisions);
                    }
                }
            } else {
                if (!first_point.is_on_curve) {
                    first_point.is_on_curve = true;
                    first_point.x = (first_point.x + second_point.x) / 2.0f;
                    first_point.y = (first_point.y + second_point.y) / 2.0f;
                }

                if (!third_point.is_on_curve) {
                    third_point.is_on_curve = true;
                    third_point.x = (second_point.x + third_point.x) / 2.0f;
                    third_point.y = (second_point.y + third_point.y) / 2.0f;
                }

                // draw bezier first => second => third
                draw_quadratic_bezier_curve(renderer, glyph, glyph_render_bounds, first_point, second_point, third_point, subdivisions);
            }
        }

        lower_index = upper_index + 1;
    }
}

// --------------------------------------------------------------------------

void print_glyph_information(const Glyph& glyph, Uint16 glyph_index) {
    std::cout << std::endl;
    std::cout << "Glyph " << glyph_index << " data:" << std::endl;
    std::cout << "Bounds: (" << glyph.min_extents.x << ", " << glyph.min_extents.y << ") => (";
    std::cout << glyph.max_extents.x << ", " << glyph.max_extents.y << ")" << std::endl;

    std::cout << "End point indices: [";
    for (int i = 0; i < glyph.num_end_point_indices; i++) {
        std::cout << glyph.end_point_indices[i] << ((i == glyph.num_end_point_indices - 1) ? "]" : ", ");
    }
    std::cout << std::endl;

    for (int i = 0; i < glyph.num_points; i++) {
        std::cout << "Point " << i << " is " << (glyph.points[i].is_on_curve ? "ON curve : " : "OFF curve: ");
        std::cout << "(" << glyph.points[i].x << ", " << glyph.points[i].y << ")" << std::endl;
    }
}

// --------------------------------------------------------------------------

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TTF_FONT_FILE" << std::endl;
        return 1;
    }

    std::string font_file_name = argv[1];
    Font font(font_file_name);

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

    // --- main loop ---

    DrawMethod draw_method = DrawMethod::POINTS;

    bool previous_was_left_arrow_pressed = false;
    bool previous_was_right_arrow_pressed = false;

    Uint16 current_glyph_index = 0;
    Glyph current_glyph = font.get_glyph(current_glyph_index);

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

        Uint16 next_glyph_index = current_glyph_index;

        const bool* keyboard = SDL_GetKeyboardState(nullptr);
        bool current_was_left_arrow_pressed = keyboard[SDL_SCANCODE_LEFT];
        bool current_was_right_arrow_pressed = keyboard[SDL_SCANCODE_RIGHT];
        if (!previous_was_left_arrow_pressed && current_was_left_arrow_pressed) {
            if (next_glyph_index == 0) {
                next_glyph_index = font.get_glyph_count() - 1;
            } else {
                next_glyph_index--;
            }
        } else if (!previous_was_right_arrow_pressed && current_was_right_arrow_pressed) {
            next_glyph_index++;
            if (next_glyph_index >= font.get_glyph_count()) {
                next_glyph_index = 0;
            }
        }

        if (next_glyph_index != current_glyph_index) {
            current_glyph_index = next_glyph_index;

            current_glyph.destroy();
            current_glyph = font.get_glyph(current_glyph_index);

        }

        previous_was_left_arrow_pressed = current_was_left_arrow_pressed;
        previous_was_right_arrow_pressed = current_was_right_arrow_pressed;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        if (draw_method == DrawMethod::POINTS) {
            draw_glyph_points(renderer, current_glyph, window_width, window_height, 20);
        } else if (draw_method == DrawMethod::LINES) {
            draw_glyph_lines(renderer, current_glyph, window_width, window_height, 20);
        } else if (draw_method == DrawMethod::CONTOURS) {
            draw_glyph_contours(renderer, current_glyph, window_width, window_height, 20, 10);
        }

        SDL_RenderPresent(renderer);
    }

    // --- cleanup ---

    current_glyph.destroy();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
