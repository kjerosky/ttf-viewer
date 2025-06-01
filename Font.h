#ifndef FONT_H
#define FONT_H

#include <SDL3/SDL.h>
#include <string>
#include <fstream>

class Font {

public:

    Font(const std::string& font_file_name);
    ~Font();

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

    void initialize(const std::string& font_file_name);
    Uint32 read_uint32_from_big_endian_file(const std::vector<Uint8> file, int location);
    Uint16 read_uint16_from_big_endian_file(const std::vector<Uint8> file, int location);
    void print_table_metadata(const Offset_subtable& offset_subtable, const Table* tables);
};

#endif
