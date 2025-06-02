#include "Font.h"

#include <fstream>
#include <iostream>

Font::Font(const std::string& font_file_name) {
    initialize(font_file_name);
}

// --------------------------------------------------------------------------

Font::~Font() {
    // nothing to do for now
}

// --------------------------------------------------------------------------
Uint32 Font::read_uint32_from_big_endian_file(const std::vector<Uint8> file_contents, int location) {
    return
        (static_cast<Uint32>(file_contents[location]) << 24) |
        (static_cast<Uint32>(file_contents[location + 1]) << 16) |
        (static_cast<Uint32>(file_contents[location + 2]) << 8) |
        static_cast<Uint32>(file_contents[location + 3])
    ;
}

// --------------------------------------------------------------------------

Uint16 Font::read_uint16_from_big_endian_file(const std::vector<Uint8> file_contents, int location) {
    return
        (static_cast<Uint16>(file_contents[location]) << 8) |
        static_cast<Uint16>(file_contents[location + 1])
    ;
}

// --------------------------------------------------------------------------

void Font::initialize(const std::string& font_file_name) {
    std::ifstream file(font_file_name, std::ifstream::binary);
    file.unsetf(std::ios::skipws);

    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    font_file_contents.resize(file_size);
    file.read((char*)font_file_contents.data(), font_file_contents.size());

    int file_location = 0;

    Offset_subtable offset_subtable;
    offset_subtable.scaler_type = read_uint32_from_big_endian_file(font_file_contents, file_location);
    file_location += 4;
    offset_subtable.num_tables = read_uint16_from_big_endian_file(font_file_contents, file_location);
    file_location += 2;
    offset_subtable.search_range = read_uint16_from_big_endian_file(font_file_contents, file_location);
    file_location += 2;
    offset_subtable.entry_selector = read_uint16_from_big_endian_file(font_file_contents, file_location);
    file_location += 2;
    offset_subtable.range_shift = read_uint16_from_big_endian_file(font_file_contents, file_location);
    file_location += 2;

    Table* tables = new Table[offset_subtable.num_tables];
    for (int i = 0; i < offset_subtable.num_tables; i++) {
        char tag[] = "XXXX";
        for (int j = 0; j < 4; j++) {
            tag[j] = font_file_contents[file_location++];
        }

        tables[i].tag = tag;
        tables[i].checksum = read_uint32_from_big_endian_file(font_file_contents, file_location);
        file_location += 4;
        tables[i].offset = read_uint32_from_big_endian_file(font_file_contents, file_location);
        file_location += 4;
        tables[i].length = read_uint32_from_big_endian_file(font_file_contents, file_location);
        file_location += 4;

        table_name_to_offset[tables[i].tag] = tables[i].offset;
    }

    print_table_metadata(offset_subtable, tables);
    std::cout << "--------------------------------------------------------------------------" << std::endl;

    Uint32 maxp_offset = table_name_to_offset["maxp"];
    Uint16 num_glyphs = read_uint16_from_big_endian_file(font_file_contents, maxp_offset + 4);
    std::cout << "Number of glyphs: " << num_glyphs << std::endl;


    delete[] tables;
}

// --------------------------------------------------------------------------

Glyph Font::get_glyph() {
    // For now, let's just read glyph 0 data.

    Glyph glyph;

    Uint32 glyf_offset = table_name_to_offset["glyf"];
    Uint32 file_location = glyf_offset;

    Sint16 num_contours = static_cast<Sint16>(read_uint16_from_big_endian_file(font_file_contents, file_location));
    file_location += 2;
    Sint16 x_min = static_cast<Sint16>(read_uint16_from_big_endian_file(font_file_contents, file_location));
    file_location += 2;
    Sint16 y_min = static_cast<Sint16>(read_uint16_from_big_endian_file(font_file_contents, file_location));
    file_location += 2;
    Sint16 x_max = static_cast<Sint16>(read_uint16_from_big_endian_file(font_file_contents, file_location));
    file_location += 2;
    Sint16 y_max = static_cast<Sint16>(read_uint16_from_big_endian_file(font_file_contents, file_location));
    file_location += 2;

    glyph.min_extents.x = x_min;
    glyph.min_extents.y = y_min;
    glyph.max_extents.x = x_max;
    glyph.max_extents.y = y_max;

    glyph.num_end_point_indices = num_contours;
    glyph.end_point_indices = new Uint32[num_contours];

    Uint16 max_contour_end_point_index = 0;
    for (int i = 0; i < num_contours; i++) {
        Uint16 contour_end_point_index = read_uint16_from_big_endian_file(font_file_contents, file_location);
        file_location += 2;

        glyph.end_point_indices[i] = contour_end_point_index;

        if (contour_end_point_index > max_contour_end_point_index) {
            max_contour_end_point_index = contour_end_point_index;
        }
    }

    Uint16 num_points = max_contour_end_point_index + 1;
    glyph.num_points = num_points;
    glyph.points = new GlyphPoint[num_points];

    Uint16 instruction_length = read_uint16_from_big_endian_file(font_file_contents, file_location);
    file_location += 2;

    // We'll skip instructions for now.
    file_location += instruction_length;

    std::vector<Uint8> flags;
    for (int i = 0; i < num_points; i++) {
        Uint8 flag = font_file_contents[file_location++];
        flags.push_back(flag);

        glyph.points[i].is_on_curve = (flag & 0x01) != 0;

        if (flag & 0x08) {
            Uint16 additional_times_flag_is_repeated = font_file_contents[file_location++];
            for (int j = 0; j < additional_times_flag_is_repeated; j++) {
                flags.push_back(flag);

                i++;
                glyph.points[i].is_on_curve = (flag & 0x01) != 0;
            }
        }
    }

    for (int i = 0; i < num_points; i++) {
        Uint8 flag = flags[i];

        Sint16 x_coordinate;
        if (flag & 0x02) {
            x_coordinate = font_file_contents[file_location++];

            if ((flag & 0x10) == 0) {
                x_coordinate = -x_coordinate;
            }
        } else {
            if ((flag & 0x10)) {
                x_coordinate = 0;
            } else {
                x_coordinate = read_uint16_from_big_endian_file(font_file_contents, file_location);
                file_location += 2;
            }
        }

        if (i == 0) {
            glyph.points[i].x = x_coordinate;
        } else {
            glyph.points[i].x = glyph.points[i - 1].x + x_coordinate;
        }
    }

    for (int i = 0; i < num_points; i++) {
        Uint8 flag = flags[i];

        Sint16 y_coordinate;
        if (flag & 0x04) {
            y_coordinate = font_file_contents[file_location++];

            if ((flag & 0x20) == 0) {
                y_coordinate = -y_coordinate;
            }
        } else {
            if ((flag & 0x20)) {
                y_coordinate = 0;
            } else {
                y_coordinate = read_uint16_from_big_endian_file(font_file_contents, file_location);
                file_location += 2;
            }
        }

        if (i == 0) {
            glyph.points[i].y = y_coordinate;
        } else {
            glyph.points[i].y = glyph.points[i - 1].y + y_coordinate;
        }
    }

    return glyph;
}

// --------------------------------------------------------------------------

void Font::print_table_metadata(const Offset_subtable& offset_subtable, const Table* tables) {
    std::cout << "Font directory:" << std::endl;
    std::cout << "Scalar type: " << offset_subtable.scaler_type << std::endl;
    std::cout << "Number of tables: " << offset_subtable.num_tables << std::endl;
    std::cout << "Search range: " << offset_subtable.search_range << std::endl;
    std::cout << "Entry selector: " << offset_subtable.entry_selector << std::endl;
    std::cout << "Range shift: " << offset_subtable.range_shift << std::endl;
    std::cout << "--------------------------------------------------------------------------" << std::endl;

    for (int i = 0; i < offset_subtable.num_tables; i++) {
        std::cout << "Table " << i + 1 << ": ";
        std::cout << "Tag: " << tables[i].tag << "    ";
        std::cout << "Checksum: " << tables[i].checksum << "    ";
        std::cout << "Offset: " << tables[i].offset << "    ";
        std::cout << "Length: " << tables[i].length << std::endl;
    }
}
