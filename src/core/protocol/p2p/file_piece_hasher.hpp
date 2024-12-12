#pragma once

#ifndef FILE_PIECE_HASHER_HPP_laiin
#define FILE_PIECE_HASHER_HPP_laiin

#include <iostream>
#include <vector>

namespace laiin {

struct FilePiece {
    std::string hash;
    size_t bytes = 0;
    std::vector<unsigned char> data;
};

class FilePieceHasher {
public:
    FilePieceHasher(size_t piece_size);
    std::vector<FilePiece> hash_file(const std::string& filename);
private:
    size_t piece_size;
};

}
#endif
