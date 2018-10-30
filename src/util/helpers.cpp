#include "util/helpers.h"

#define PHYFSPP_IMPL
#include <physfs.hpp>
#include "util/logging.h"

std::string helpers::readToString(const std::string& filename)
{
    PhysFS::ifstream stream(filename);
    return std::string(std::istreambuf_iterator<char>(stream),
                       std::istreambuf_iterator<char>());
}