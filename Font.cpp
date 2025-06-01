#include "Font.h"

#include <fstream>
#include <vector>
#include <iostream>

Font::Font(const std::string& font_file_name) {
    initialize(font_file_name);
}

// --------------------------------------------------------------------------

Font::~Font() {
    // nothing to do for now
}

// --------------------------------------------------------------------------
Uint32 Font::read_uint32_from_big_endian_file(const std::vector<Uint8> file, int location) {
    return
        (static_cast<Uint32>(file[location]) << 24) |
        (static_cast<Uint32>(file[location + 1]) << 16) |
        (static_cast<Uint32>(file[location + 2]) << 8) |
        static_cast<Uint32>(file[location + 3])
    ;
}

// --------------------------------------------------------------------------

Uint16 Font::read_uint16_from_big_endian_file(const std::vector<Uint8> file, int location) {
    return
        (static_cast<Uint16>(file[location]) << 8) |
        static_cast<Uint16>(file[location + 1])
    ;
}

// --------------------------------------------------------------------------

void Font::initialize(const std::string& font_file_name) {
    std::ifstream file(font_file_name, std::ifstream::binary);
    file.unsetf(std::ios::skipws);

    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<Uint8> font_file_contents(file_size);
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
    }

    print_table_metadata(offset_subtable, tables);

    delete[] tables;
}

// --------------------------------------------------------------------------

void Font::print_table_metadata(const Offset_subtable& offset_subtable, const Table* tables) {
    std::cout << "Font directory:" << std::endl;
    std::cout << std::endl;
    std::cout << "Scalar type: " << offset_subtable.scaler_type << std::endl;
    std::cout << "Number of tables: " << offset_subtable.num_tables << std::endl;
    std::cout << "Search range: " << offset_subtable.search_range << std::endl;
    std::cout << "Entry selector: " << offset_subtable.entry_selector << std::endl;
    std::cout << "Range shift: " << offset_subtable.range_shift << std::endl;

    for (int i = 0; i < offset_subtable.num_tables; i++) {
        std::cout << "--------------------------------------------------------------------------" << std::endl;
        std::cout << "Table " << i + 1 << ": ";
        std::cout << "Tag: " << tables[i].tag << "    ";
        std::cout << "Checksum: " << tables[i].checksum << "    ";
        std::cout << "Offset: " << tables[i].offset << "    ";
        std::cout << "Length: " << tables[i].length << std::endl;
    }
}
