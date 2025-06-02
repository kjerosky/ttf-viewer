#ifndef FONT_H
#define FONT_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <map>

struct Coordinate {
    Sint16 x;
    Sint16 y;
};

struct GlyphPoint {
    Sint16 x;
    Sint16 y;
    bool is_on_curve;
};

struct Glyph {
    Coordinate min_extents;
    Coordinate max_extents;
    Uint16 num_end_point_indices;
    Uint32* end_point_indices;
    Uint16 num_points;
    GlyphPoint* points;

    void destroy() {
        delete[] end_point_indices;
        delete[] points;
    }
};

// --------------------------------------------------------------------------

class Font {

public:

    Font(const std::string& font_file_name);
    ~Font();

    Uint16 get_glyph_count();
    Glyph get_glyph(Uint16 glyph_index);

private:

    struct Table {
        std::string tag;
        Uint32 checksum;
        Uint32 offset;
        Uint32 length;
    };

    struct Offset_subtable {
        Uint32 scaler_type;
        Uint16 num_tables;
        Uint16 search_range;
        Uint16 entry_selector;
        Uint16 range_shift;
    };

    std::vector<Uint8> font_file_contents;
    std::map<std::string, Uint32> table_name_to_offset;

    Uint16 glyph_count;
    Uint32* glyph_offsets;

    void initialize(const std::string& font_file_name);
    Uint32 read_uint32_from_big_endian_file(const std::vector<Uint8> file_contents, int location);
    Uint16 read_uint16_from_big_endian_file(const std::vector<Uint8> file_contents, int location);
    void print_table_metadata(const Offset_subtable& offset_subtable, const Table* tables);
};

#endif
