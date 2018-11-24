#include <glm/glm.hpp>

#include <type_traits>

#include "graphics/spritepool.h"
#include "graphics/debug.h"
#include "util/logging.h"
#include "util/helpers.h"


graphics::SpritePool::SpritePool ()
    : visibleSprites(unsigned(-1))
{

}

graphics::SpritePool::~SpritePool () {
}

void graphics::SpritePool::init (const graphics::shader& spriteShader, int texture_unit)
{
    mesh.bind();
    mesh.addBuffer(std::vector<glm::vec3>{
            {-0.5f, 2.0f, 0.0f},
            {-0.5f, 0.0f, 0.0f},
            {0.5f,  2.0f, 0.0f},
            {0.5f,  0.0f, 0.0f}
        }, true);
    mesh.addBuffer(std::vector<glm::vec2>{
            {0.0f, 0.0f},
            {0.0f, 1.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f}
        });

    glGenBuffers(1, &tbo);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo);
    glGenTextures(1, &tbo_tex);
    glBindTexture(GL_TEXTURE_BUFFER, tbo_tex);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec4), nullptr, GL_STREAM_DRAW); // This will get replaced on the first update
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    checkErrors();

    u_tbo_tex = spriteShader.uniform("u_tbo_tex");
    spriteShader.uniform("u_texture").set(texture_unit);

    checkErrors();

    spriteCount = 0;
}

// TODO: avoid copying data by using some kind of quad tree or other spatially aware data structure. Also, set spriteCount as a static max on-screen limit (1k sprites?)
void graphics::SpritePool::update (const std::vector<Sprite>& sprites)
{
    if (spriteCount != sprites.size()) {
        if (spriteCount != 0) { // If this isn't the first update, then warn that the size has changed
            warn("SpritePool inited for {} sprites but {} sprites updated - this may have a performance impact", spriteCount, sprites.size());
        }
        spriteCount = unsigned(sprites.size());
        unsortedBuffer.reserve(spriteCount);
        sortedBuffer.reserve(spriteCount);
    }
    unsortedBuffer.clear();
    std::copy(sprites.begin(), sprites.end(), std::back_inserter(unsortedBuffer));
}

inline void sse_cull_spheres(std::vector<graphics::Sprite>::const_iterator sphere_data, std::size_t num_objects, std::vector<int>& culling_results, const std::array<glm::vec4, 6>& frustum_planes)
{
    trace_fn();
    typename std::aligned_storage<sizeof(std::uint32_t), 16>::type results[4];

    //to optimize calculations we gather xyzw elements in separate vectors
    __m128 zero_v = _mm_setzero_ps();
    __m128 frustum_planes_x[6];
    __m128 frustum_planes_y[6];
    __m128 frustum_planes_z[6];
    __m128 frustum_planes_d[6];
    for (std::size_t i = 0; i < 6; ++i) {
        frustum_planes_x[i] = _mm_set1_ps(frustum_planes[i].x);
        frustum_planes_y[i] = _mm_set1_ps(frustum_planes[i].y);
        frustum_planes_z[i] = _mm_set1_ps(frustum_planes[i].z);
        frustum_planes_d[i] = _mm_set1_ps(frustum_planes[i].w);
    }
    //we process 4 objects per step
    float temp[4][4];
    for (std::size_t i = 0; i < num_objects; i += 4) {
        //load bounding sphere data
        for (std::size_t j = 0; j < 4; ++j) {
            const graphics::Sprite& sprite = *sphere_data++;
            temp[0][j] = sprite.position.x;
            temp[1][j] = sprite.position.y;
            temp[2][j] = sprite.position.z;
            temp[3][j] = 1.0f; // diameter of 2.0f
        }
        __m128 spheres_pos_x  = _mm_load_ps(temp[0]);
        __m128 spheres_pos_y = _mm_load_ps(temp[1]);
        __m128 spheres_pos_z = _mm_load_ps(temp[2]);
        __m128 spheres_radius = _mm_load_ps(temp[3]);

        __m128 spheres_neg_radius = _mm_sub_ps(zero_v, spheres_radius); // negate all elements
        __m128 intersection_res = _mm_setzero_ps();
        for (int j = 0; j < 6; ++j) { //plane index
            //1. calc distance to plane dot(sphere_pos.xyz, plane.xyz) + plane.w
            //2. if distance < sphere radius, then sphere outside frustum
            __m128 dot_x = _mm_mul_ps(spheres_pos_x, frustum_planes_x[j]);
            __m128 dot_y = _mm_mul_ps(spheres_pos_y, frustum_planes_y[j]);
            __m128 dot_z = _mm_mul_ps(spheres_pos_z, frustum_planes_z[j]);
            __m128 sum_xy = _mm_add_ps(dot_x, dot_y);
            __m128 sum_zw = _mm_add_ps(dot_z, frustum_planes_d[j]);
            __m128 distance_to_plane = _mm_add_ps(sum_xy, sum_zw);
            __m128 plane_res = _mm_cmple_ps(distance_to_plane, spheres_neg_radius); //dist < -sphere_r ?
            intersection_res = _mm_or_ps(intersection_res, plane_res); //if yes - sphere behind the plane & outside frustum
        }
        //store result
        __m128i intersection_res_i = _mm_cvtps_epi32(intersection_res);
        _mm_store_si128(reinterpret_cast<__m128i*>(&results), intersection_res_i);
        std::uint32_t* r = reinterpret_cast<std::uint32_t*>(&results[0]);
        for (std::size_t j = 0; j < 4; ++j) {
            culling_results.push_back(*r++);
        }
    }
}

void cull_spheres (std::vector<graphics::Sprite>::const_iterator sphere_data, std::size_t num_objects, std::vector<int>& culling_results, const std::array<glm::vec4, 6>& frustum_planes)
{
    int res;
    for (std::size_t idx = 0; idx < num_objects; ++idx) {
        res = 0;
        const graphics::Sprite& sprite = *sphere_data++;
        const glm::vec3& pos = sprite.position;
        //test all 6 frustum planes
        for (int i = 0; i < 6; i++)
        {
            //calculate distance from sphere center to plane.
            //if distance larger then sphere radius - sphere is outside frustum
            if (frustum_planes[i].x * pos.x + frustum_planes[i].y * pos.y + frustum_planes[i].z * pos.z + frustum_planes[i].w + 1.0f <= 0)
                res = 1;
        }
        culling_results.push_back(res);
    }
}

std::size_t graphics::SpritePool::cull_sprites (const math::frustum& frustum, std::vector<graphics::Sprite>& positions)
{
    trace_fn();
    // perform frustum culling and package sprite data for rendering
    std::size_t num_objects = positions.size();
    // make sure num_objects is multiple of 4. Pad with null objects if not.
    while ((positions.size() & 0x3) != 0) {
        positions.push_back({});
    }
    size_t num_objects_to_cull = positions.size();
    culling_results.clear();
    culling_results.reserve(num_objects_to_cull);
    // sse_cull_spheres(positions.begin(), num_objects_to_cull, culling_results, frustum.planes);
    cull_spheres(positions.begin(), num_objects_to_cull, culling_results, frustum.planes);
    // Remove null objects, if any
    std::size_t to_remove = positions.size() - num_objects;
    for (std::size_t i = to_remove; i; --i) {
        positions.pop_back();
    }
    // Remove culled sprites
    for (std::size_t i = 0; i < num_objects; ++i) {
        if (culling_results[i]) {
            helpers::remove(positions, i);
        }
    }
    // info("Number of sprites culled: {} of {}", num_objects - positions.size(), num_objects);
    return positions.size();
}


void graphics::SpritePool::render (const math::frustum& frustum)
{
    // visibleSprites = cull_sprites(frustum, unsortedBuffer);
    visibleSprites = unsortedBuffer.size();

    glActiveTexture(GL_TEXTURE0 + 6);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo);
    glBindTexture(GL_TEXTURE_BUFFER, tbo_tex);
    // Orphan old buffer and then load data into new buffer
    glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec4) * spriteCount, nullptr, GL_STREAM_DRAW);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(glm::vec4) * spriteCount, reinterpret_cast<const float*>(unsortedBuffer.data()), GL_STREAM_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, tbo);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    debug("Rendering {} visible sprites ({} total)", visibleSprites, spriteCount);

    u_tbo_tex.set(6);
    mesh.draw(visibleSprites);
    checkErrors();
}