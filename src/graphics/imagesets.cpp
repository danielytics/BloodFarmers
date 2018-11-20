#include "graphics/imagesets.h"

#include <spdlog/fmt/fmt.h>
#include <cpptoml.h>

#include "util/helpers.h"
#include "util/logging.h"
#include "graphics/textures.h"

graphics::Imagesets::Imagesets()
{

}

graphics::Imagesets::~Imagesets()
{
    unload();
}

void graphics::Imagesets::load (const entt::hashed_string& id, bool textureFiltering, const std::vector<std::string>& filenames)
{
    info("Loading {} images for tileset '{}'", filenames.size(), id);
    int imageset_idx = texture_arrays.size();
    imagesets[id] = imageset_idx;
    glActiveTexture(GL_TEXTURE0 + imageset_idx);
    auto texture_array = textures::loadArray(textureFiltering, filenames);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    texture_arrays.push_back(texture_array);
}

void graphics::Imagesets::load (const std::string& configFilename)
{
    info("Loading imagesets from '{}'", configFilename);
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(configFilename));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("imageset");
        for (const auto& imageset_table : *tarr) {
            auto name = imageset_table->get_as<std::string>("id");
            auto filtering = imageset_table->get_as<bool>("filtering");
            
            std::vector<std::string> files;
            auto images = imageset_table->get_table_array("images");
            for (const auto& image_table : *images) {
                auto directory = image_table->get_as<std::string>("directory");
                auto pattern = image_table->get_as<std::string>("file-pattern");
                auto range = image_table->get_array_of<int64_t>("file-range");
                for (auto i = (*range)[0]; i <= (*range)[1]; ++i) {
                    files.push_back(*directory + fmt::format(*pattern, i));
                }
            }
            load(entt::hashed_string{name->data()}, *filtering, files);
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
}

void graphics::Imagesets::unload ()
{
    glDeleteTextures(texture_arrays.size(), texture_arrays.data());
    texture_arrays.clear();
    imagesets.clear();
}
