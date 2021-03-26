#include "SDL.h"
#include "SDL_main.h"
#include "sdl_spine.h"

void InitVideo();
void InitData();
void UpdateData(float dt);
void CleanUp();
bool Idle();
void DrawAll();

[[noreturn]] void ProgramDie(const char* msg = NULL);

//--------------------------------------------------
// MAIN
//--------------------------------------------------
int main(int argc, char* argv[])
{
    InitVideo();
    InitData();

    while (Idle());

    CleanUp();

    return 0;
}
//--------------------------------------------------

const int WINDOW_W = 1280;
const int WINDOW_H = 720;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
spine_demo::SpineAnimation* spine_animation = nullptr;
spSkeletonData* m_skeleton_data = nullptr;
spAtlas* atlas = nullptr;

void InitVideo()
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        ProgramDie("Cannon init video");

    window = SDL_CreateWindow("TEST", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!window)
        ProgramDie();
    const int renderer_driver_index = -1;
    renderer = SDL_CreateRenderer(window, renderer_driver_index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        ProgramDie();

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Renderer: %s", info.name);
}

void InitData()
{
    auto atlas_path = "data/spineboy-pma.atlas";
    auto binary_path = "data/spineboy-pro.skel";
    auto json_path = "data/spineboy-pro.json";
    auto default_scale = 1.f;

    atlas = spAtlas_createFromFile(atlas_path, 0);
    //m_skeleton_data = spine::SpineAnimation::ReadSkeletonBinaryData(binary_path, atlas, default_scale);//using binary format
    m_skeleton_data = spine_demo::SpineAnimation::ReadSkeletonJsonData(json_path, atlas, default_scale);//using json format  

    spine_animation = new spine_demo::SpineAnimation(m_skeleton_data);
    spine_animation->SetPos(600, 700);
    spine_animation->SetUsePremultipliedAlpha(true);

    auto animation = "walk"; //"jump", "run";
    spine_animation->RunAnimation(animation, true, true);
}

void UpdateData(float dt)
{
    spine_animation->Update(dt);
}

void CleanUp()
{
    if (atlas)
    {
        spAtlas_dispose(atlas);
        atlas = nullptr;
    }

    if (m_skeleton_data)
    {
        spSkeletonData_dispose(m_skeleton_data);
        m_skeleton_data = nullptr;
    }

    if (spine_animation)
    {
        delete spine_animation;
        spine_animation = nullptr;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

bool Idle()
{
    SDL_Event event;
    const int MAX_EVENTS = 100;
    SDL_Event events_array[MAX_EVENTS + 1];

    auto ticks = SDL_GetTicks();
    while (true)
    {
        auto cur_tick = SDL_GetTicks();
        UpdateData((cur_tick - ticks) / 1000.f);
        ticks = cur_tick;

        DrawAll();
        if (!SDL_WaitEventTimeout(&event, 0))
            continue;

        int count = 0;
        events_array[0] = event;
        count = SDL_PeepEvents(events_array + 1, MAX_EVENTS, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);

        for (int i = 0; i < count + 1; ++i)
        {
            auto& ev = events_array[i];
            switch (ev.type)
            {
            case SDL_QUIT:
                return false;
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDL_KEYDOWN:
                spine_animation->RunAnimation("jump", true, false);
                break;
            }
        }
        SDL_Delay(1);
    }
}

void DrawAll()
{
    static int frames = 0;
    frames++;
    static auto last_tick = SDL_GetTicks();
    auto current_tick = SDL_GetTicks();
    if ((current_tick - last_tick) >= 1000)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "FPS: %d", frames);
        last_tick = current_tick;
        frames = 0;
    }

    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 0);
    SDL_RenderClear(renderer);

    spine_animation->Draw(renderer);
    
    SDL_RenderPresent(renderer);
}

void ProgramDie(const char* msg)
{
    auto sdl_msg = SDL_GetError();
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL: %s", sdl_msg);
    if (msg)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "USER: %s", msg);
    }
    CleanUp();
    system("pause");
    exit(-1);
}

SDL_Renderer* GetSdlRender()
{
    return renderer;
}