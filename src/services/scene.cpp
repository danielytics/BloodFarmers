
#include <cpptoml.h>
#include <entt/entt.hpp>

#include "types.h"
#include "components.h"
#include "util/helpers.h"
/*
void loadScene (ecs::registry_t& registry, const std::string& config_file)
{
    try {
        std::istringstream iss;
        iss.str(helpers::readToString(config_file));
        cpptoml::parser parser{iss};
        std::shared_ptr<cpptoml::table> config = parser.parse();
        auto tarr = config->get_table_array("entity");
        for (const auto& table : *tarr) {
            auto id = table->get_as<std::string>("id");
            auto lifecycle = table->get_as<std::string>("lifecycle");
            auto type = table->get_as<std::string>("type");
            auto requests = table->get_as<std::string>("requests");
            auto buffers = table->get_as<int64_t>("buffers");
            auto buffer = table->get_table("buffer");
            auto alignment = buffer->get_as<int64_t>("alignment");
            auto size = buffer->get_as<int64_t>("size");
            auto units = buffer->get_as<std::string>("units");

            std::uint32_t num_bytes = *size;
            std::uint32_t element_size = resources.sizeOf(entt::hashed_string{type->data()});
            if (*units == "b") {
                // No-op
            } else if (*units == "kb") {
                num_bytes *= 1024;
            } else if (*units == "mb") {
                num_bytes *= 1024 * 1024; 
            } else if (*units == "elements") {
                num_bytes = num_bytes * element_size;
            }
            
            // Register the buffer
            resources.registerResource({
                entt::hashed_string{id->data()},        // id
                entt::hashed_string{lifecycle->data()}, // lifecycle
                entt::hashed_string{requests->data()},  // request_type
                "buffer-allocator"_hs,                  // allocator
                entt::hashed_string{type->data()},      // contained_type
                std::uint32_t(*alignment),              // buffer alignment
                num_bytes,                              // size
                std::uint32_t(*buffers),                // num_buffers
            });
        }
    }
    catch (const cpptoml::parse_exception& e) {
        fatal("Parsing failed: {}", e.what());
    }
}
*/