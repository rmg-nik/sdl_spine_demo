#pragma once

#define SPINE_SHORT_NAMES

#include "spine/spine.h"
#include "spine/extension.h"

#include <string>
#include <vector>

_SP_ARRAY_DECLARE_TYPE(spColorArray, spColor)

struct SDL_Renderer;

namespace spine_demo
{
    class SpineAnimation 
    {

    public:


    public:


        SpineAnimation(SkeletonData* m_skeleton, AnimationStateData* stateData = 0);

        ~SpineAnimation();

        spTrackEntry* RunAnimation(const char* animation, bool reset, bool repeat);

        void Update(float deltaTime);

        void Draw(SDL_Renderer* renderer);

        void SetUsePremultipliedAlpha(bool usePMA) { m_use_pma = usePMA; };

        bool GetUsePremultipliedAlpha() { return m_use_pma; };

        static SkeletonData* ReadSkeletonJsonData(const char* filename, Atlas* atlas, float scale);

        static SkeletonData* ReadSkeletonBinaryData(const char* filename, Atlas* atlas, float scale);

        void SetPos(float x, float y);

    private:

        bool m_owns_animation_state_data;
        float* m_world_vertices;
        spSkeletonClipping* m_clipper;
        bool m_use_pma;
        Skeleton* m_skeleton;
        AnimationState* m_state;
        float m_time_scale;
        SkeletonData* m_skeleton_data;
    };

}
