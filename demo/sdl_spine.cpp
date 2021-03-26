#include "sdl_spine.h"
#include "stb/stb_image.h"
#include "SDL.h"
#include <algorithm>
#include <stdexcept>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

using namespace spine_demo;

extern SDL_Renderer* GetSdlRender();

void _spAtlasPage_createTexture(AtlasPage* self, const char* path) {

    int req_format = STBI_rgb_alpha;
    int width, height, orig_format;
    unsigned char* data = stbi_load(path, &width, &height, &orig_format, req_format);
    if (data == NULL) {
        SDL_Log("Loading image failed: %s", stbi_failure_reason());
        exit(1);
    }

    int depth, pitch;
    Uint32 pixel_format;
    if (req_format == STBI_rgb) {
        depth = 24;
        pitch = 3 * width; // 3 bytes per pixel * pixels per row
        pixel_format = SDL_PIXELFORMAT_RGB24;
    }
    else { // STBI_rgb_alpha (RGBA)
        depth = 32;
        pitch = 4 * width;
        pixel_format = SDL_PIXELFORMAT_RGBA32;
    }

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom((void*)data, width, height,
        depth, pitch, pixel_format);

    if (surf == NULL) {
        SDL_Log("Creating surface failed: %s", SDL_GetError());
        stbi_image_free(data);
        exit(1);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(GetSdlRender(), surf);

    self->rendererObject = texture;
    self->width = surf->w;
    self->height = surf->h;

    SDL_RenderCopy(GetSdlRender(), texture, NULL, NULL);
    SDL_FreeSurface(surf);
    stbi_image_free(data);
}

void _spAtlasPage_disposeTexture(AtlasPage* self) {
    if (self->rendererObject != NULL)
        SDL_DestroyTexture((SDL_Texture*)self->rendererObject);
}

char* _spUtil_readFile(const char* path, int* length) {

    char* data;
    SDL_RWops* file = SDL_RWFromFile(path, "rb");
    if (!file)
        return 0;

    SDL_RWseek(file, 0, SEEK_END);
    *length = (int)SDL_RWtell(file);
    SDL_RWseek(file, 0, SEEK_SET);

    data = MALLOC(char, *length);
    SDL_RWread(file, data, 1, *length);
    SDL_RWclose(file);

    return data;

}

namespace
{
    SDL_BlendMode sdl_blend_normal = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_SRC_ALPHA,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_additive = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_SRC_ALPHA,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ZERO,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_multiply = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_DST_COLOR,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_DST_ALPHA,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_normalPma = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_additivePma = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ZERO,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_multiplyPma = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_DST_COLOR,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_DST_ALPHA,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        SDL_BLENDOPERATION_ADD);

    SDL_BlendMode sdl_blend_screenPma = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
        SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE,
        SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
        SDL_BLENDOPERATION_ADD);

}
SpineAnimation::SpineAnimation(SkeletonData* m_skeleton_data, AnimationStateData* stateData)
{
    this->m_skeleton_data = m_skeleton_data;
    m_time_scale = 1;

    m_world_vertices = 0;
    m_clipper = 0;

    Bone_setYDown(true);
    m_world_vertices = MALLOC(float, SPINE_MESH_VERTEX_COUNT_MAX);
    m_skeleton = Skeleton_create(m_skeleton_data);

    m_owns_animation_state_data = stateData == 0;
    if (m_owns_animation_state_data) stateData = AnimationStateData_create(m_skeleton_data);

    m_state = AnimationState_create(stateData);

    m_clipper = spSkeletonClipping_create();

    m_use_pma = true;
}

SpineAnimation::~SpineAnimation() {
    FREE(m_world_vertices);
    if (m_owns_animation_state_data) 
        AnimationStateData_dispose(m_state->data);
    AnimationState_dispose(m_state);
    Skeleton_dispose(m_skeleton);
    spSkeletonClipping_dispose(m_clipper);
}

SkeletonData* SpineAnimation::ReadSkeletonJsonData(const char* filename, Atlas* atlas, float scale) {
    SkeletonJson* json = SkeletonJson_create(atlas);
    json->scale = scale;
    SkeletonData* m_skeleton_data = SkeletonJson_readSkeletonDataFile(json, filename);
    if (!m_skeleton_data) {
        printf("%s\n", json->error);
        exit(0);
    }
    SkeletonJson_dispose(json);
    return m_skeleton_data;
}

SkeletonData* SpineAnimation::ReadSkeletonBinaryData(const char* filename, Atlas* atlas, float scale) {
    SkeletonBinary* binary = SkeletonBinary_create(atlas);
    binary->scale = scale;
    SkeletonData* m_skeleton_data = SkeletonBinary_readSkeletonDataFile(binary, filename);
    if (!m_skeleton_data) {
        printf("%s\n", binary->error);
        exit(0);
    }
    SkeletonBinary_dispose(binary);
    return m_skeleton_data;
}

void SpineAnimation::SetPos(float x, float y)
{
    m_skeleton->x = x;
    m_skeleton->y = y;
    Skeleton_updateWorldTransform(m_skeleton);
}

spTrackEntry* SpineAnimation::RunAnimation(const char* animation, bool resetdrawstatus, bool repeat)
{
    spTrackEntry* entry = NULL;
    if (SkeletonData_findAnimation(m_skeleton_data, animation))
    {
        if (resetdrawstatus)
        {
            Skeleton_setBonesToSetupPose(m_skeleton);
            Skeleton_setSlotsToSetupPose(m_skeleton);
            AnimationState_clearTracks(m_state);
        }

        spTrackEntry* entry = AnimationState_setAnimationByName(m_state, 0, animation, repeat);
        if (entry)
        {
        }
    }
    return entry;
}

void SpineAnimation::Update(float deltaTime) {
    Skeleton_update(m_skeleton, deltaTime);
    AnimationState_update(m_state, deltaTime * m_time_scale);
    AnimationState_apply(m_state, m_skeleton);
    Skeleton_updateWorldTransform(m_skeleton);
}

void SpineAnimation::Draw(SDL_Renderer* renderer)
{
    std::vector<SDL_Vertex> sdl_vertices;    
    sdl_vertices.reserve(SPINE_MESH_VERTEX_COUNT_MAX);

    unsigned short quadIndices[6] = { 0, 1, 2, 2, 3, 0 };

    SDL_Vertex vertex;
    SDL_Texture* texture = 0;
    SDL_Texture* nextTexture = 0;
    SDL_BlendMode blend = SDL_BlendMode::SDL_BLENDMODE_NONE;

    for (int i = 0; i < m_skeleton->slotsCount; ++i)
    {
        spSlot* slot = m_skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment)
            continue;

        float* vertices = m_world_vertices;
        int verticesCount = 0;
        float* uvs = 0;
        unsigned short* indices = 0;
        int indicesCount = 0;
        spColor* attachmentColor;

        if (attachment->type == SP_ATTACHMENT_REGION)
        {
            spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
            spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, vertices, 0, 2);
            verticesCount = 4;
            uvs = regionAttachment->uvs;
            indices = quadIndices;
            indicesCount = 6;
            nextTexture = (SDL_Texture*)((AtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;
            attachmentColor = &regionAttachment->color;

        }
        else if (attachment->type == SP_ATTACHMENT_MESH)
        {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            if (mesh->super.worldVerticesLength > SPINE_MESH_VERTEX_COUNT_MAX)
                continue;
            nextTexture = (SDL_Texture*)((AtlasRegion*)mesh->rendererObject)->page->rendererObject;
            spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, m_world_vertices, 0, 2);
            verticesCount = mesh->super.worldVerticesLength >> 1;
            uvs = mesh->uvs;
            indices = mesh->triangles;
            indicesCount = mesh->trianglesCount;
            attachmentColor = &mesh->color;
        }
        else if (attachment->type == SP_ATTACHMENT_CLIPPING)
        {
            spClippingAttachment* clip = (spClippingAttachment*)slot->attachment;
            spSkeletonClipping_clipStart(m_clipper, slot, clip);
            continue;
        }
        else
            continue;

        std::uint8_t r = static_cast<std::uint8_t>(std::clamp(255.f * m_skeleton->color.r * slot->color.r * attachmentColor->r, 0.f, 255.f));
        std::uint8_t g = static_cast<std::uint8_t>(std::clamp(255.f * m_skeleton->color.g * slot->color.g * attachmentColor->g, 0.f, 255.f));
        std::uint8_t b = static_cast<std::uint8_t>(std::clamp(255.f * m_skeleton->color.b * slot->color.b * attachmentColor->b, 0.f, 255.f));
        std::uint8_t a = static_cast<std::uint8_t>(std::clamp(255.f * m_skeleton->color.a * slot->color.a * attachmentColor->a, 0.f, 255.f));

        vertex.color.r = r;
        vertex.color.g = g;
        vertex.color.b = b;
        vertex.color.a = a;

        SDL_BlendMode nextBlend = SDL_BlendMode::SDL_BLENDMODE_NONE;
        if (!m_use_pma)
        {
            switch (slot->data->blendMode)
            {
            case SP_BLEND_MODE_NORMAL:   nextBlend = sdl_blend_normal;      break;
            case SP_BLEND_MODE_ADDITIVE: nextBlend = sdl_blend_additive;    break;
            case SP_BLEND_MODE_MULTIPLY: nextBlend = sdl_blend_multiply;    break;
            case SP_BLEND_MODE_SCREEN:   throw std::runtime_error("SP_BLEND_MODE_SCREEN need premultiplied alpha");
            default:                     nextBlend = sdl_blend_normal;  break;
            }
        }
        else
        {
            vertex.color.r = static_cast<std::uint8_t>((int)r * a / 255.f);
            vertex.color.g = static_cast<std::uint8_t>((int)g * a / 255.f);
            vertex.color.b = static_cast<std::uint8_t>((int)b * a / 255.f);

            switch (slot->data->blendMode)
            {
            case SP_BLEND_MODE_NORMAL:   nextBlend = sdl_blend_normalPma;      break;
            case SP_BLEND_MODE_ADDITIVE: nextBlend = sdl_blend_additivePma;    break;
            case SP_BLEND_MODE_MULTIPLY: nextBlend = sdl_blend_multiplyPma;    break;
            case SP_BLEND_MODE_SCREEN:   nextBlend = sdl_blend_screenPma;      break;
            default:                     nextBlend = sdl_blend_normalPma;      break;
            }
        }

        if (!sdl_vertices.empty() && (nextBlend != blend || texture != nextTexture) && texture)
        {
            SDL_SetTextureBlendMode(texture, blend);
            SDL_RenderDrawArray(renderer, texture, sdl_vertices.data(), 0, (int)sdl_vertices.size());
            sdl_vertices.clear();
        }

        blend = nextBlend;
        texture = nextTexture;

        if (spSkeletonClipping_isClipping(m_clipper))
        {
            spSkeletonClipping_clipTriangles(m_clipper, vertices, verticesCount << 1, indices, indicesCount, uvs, 2);
            vertices = m_clipper->clippedVertices->items;
            verticesCount = m_clipper->clippedVertices->size >> 1;
            uvs = m_clipper->clippedUVs->items;
            indices = m_clipper->clippedTriangles->items;
            indicesCount = m_clipper->clippedTriangles->size;
        }

        for (int j = 0; j < indicesCount; ++j)
        {
            int w, h;
            SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
            int index = indices[j] << 1;
            vertex.pos.x = vertices[index];
            vertex.pos.y = vertices[index + 1];
            vertex.tex.x = uvs[index] * w;
            vertex.tex.y = uvs[index + 1] * h;
            sdl_vertices.push_back(vertex);
        }

        spSkeletonClipping_clipEnd(m_clipper, slot);
    }

    spSkeletonClipping_clipEnd2(m_clipper);

    if (!sdl_vertices.empty())
    {
        SDL_SetTextureBlendMode(texture, blend);
        SDL_RenderDrawArray(renderer, texture, sdl_vertices.data(), 0, (int)sdl_vertices.size());
        sdl_vertices.clear();
    }
}