#include "util/helpers.h"

#define PHYFSPP_IMPL
#include <physfs.hpp>
#include "util/logging.h"

#include <exception>

std::string helpers::readToString(const std::string& filename)
{
    if (PhysFS::exists(filename)) {
        PhysFS::ifstream stream(filename);
        return std::string(std::istreambuf_iterator<char>(stream),
                        std::istreambuf_iterator<char>());
    } else {
        auto message = std::string{"File could not be read: "} + filename;
        throw std::invalid_argument(message);
    }
}