#include <SDL3/SDL.h>
#include <iostream>
#include <string>
#include <fstream>

Uint32 read_uint32_from_big_endian_file(std::ifstream& file) {
    Uint8 number[4];
    file.read(reinterpret_cast<char*>(number), 4);

    return
        (static_cast<Uint32>(number[0]) << 24) |
        (static_cast<Uint32>(number[1]) << 16) |
        (static_cast<Uint32>(number[2]) << 8) |
        static_cast<Uint32>(number[3])
    ;
}

// --------------------------------------------------------------------------

Uint16 read_uint16_from_big_endian_file(std::ifstream& file) {
    Uint8 number[2];
    file.read(reinterpret_cast<char*>(number), 2);

    return
        (static_cast<Uint16>(number[0]) << 8) |
        static_cast<Uint16>(number[1])
    ;
}

// --------------------------------------------------------------------------

void print_font_metadata(std::ifstream& font_file) {
    Uint32 scaler_type = read_uint32_from_big_endian_file(font_file);
    Uint16 num_tables = read_uint16_from_big_endian_file(font_file);
    Uint16 search_range = read_uint16_from_big_endian_file(font_file);
    Uint16 entry_selector = read_uint16_from_big_endian_file(font_file);
    Uint16 range_shift = read_uint16_from_big_endian_file(font_file);

    std::cout << "Font directory:" << std::endl;
    std::cout << std::endl;
    std::cout << "Scalar type: " << scaler_type << std::endl;
    std::cout << "Number of tables: " << num_tables << std::endl;
    std::cout << "Search range: " << search_range << std::endl;
    std::cout << "Entry selector: " << entry_selector << std::endl;
    std::cout << "Range shift: " << range_shift << std::endl;

    for (int i = 1; i <= num_tables; i++) {
        std::cout << "--------------------------------------------------------------------------" << std::endl;
        std::cout << "Table " << i << ": ";

        char tag[5];
        font_file.read(tag, 4);
        tag[4] = '\0';

        Uint32 checksum = read_uint32_from_big_endian_file(font_file);
        Uint32 offset = read_uint32_from_big_endian_file(font_file);
        Uint32 length = read_uint32_from_big_endian_file(font_file);

        std::cout << "Tag: " << tag << " / ";
        std::cout << "Checksum: " << checksum << " / ";
        std::cout << "Offset: " << offset << " / ";
        std::cout << "Length: " << length << std::endl;
    }
}

// --------------------------------------------------------------------------

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TTF_FONT_FILE" << std::endl;
        return 1;
    }

    std::string font_file_name = argv[1];
    std::ifstream font_file(font_file_name);
    if (!font_file) {
        std::cerr << "[ERROR] File \"" << font_file_name << "\" does not exist!" << std::endl;
        return 1;
    }

    print_font_metadata(font_file);
    font_file.close();

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

        SDL_RenderPresent(renderer);
    }

    // --- cleanup ---

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
