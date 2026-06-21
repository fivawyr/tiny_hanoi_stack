 #include <cmath>
#include <iostream>
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include <stack>

//info, tower_of_hanoi was the previous implementation

using namespace std;
typedef uint8_t u8;
typedef int32_t i32;
typedef int64_t i64;
typedef float_t f32;

static constexpr i32 WIDTH = 900;
static constexpr i32 HEIGHT = 600;
static constexpr i32 NUM_DISKS = 7;
static constexpr i32 NUM_RODS = 3;
static constexpr i32 ANIM_STEPS = 40;
static constexpr i32 FRAME_DELAY = 16;
static constexpr i32 ROD_W = 10;
static constexpr i32 ROD_H = 300;
static constexpr i32 BASE_H = 18;
static constexpr i32 DISK_H = 26;
static constexpr i32 DISK_W_MIN = 40;
static constexpr i32 DISK_W_MAX = 160;
static constexpr i32 BASE_Y = HEIGHT - 120;
static constexpr i32 ROD_TOP_Y = BASE_Y - ROD_H;
static constexpr i32 ROD_X[3] = {180, 450, 720};

struct Color {
    u8 r;
    u8 g;
    u8 b;
};

struct Move {
    i32 from;
    i32 to;
    i32 disk;
};

struct State {
    stack<i32> rods[NUM_RODS];
    void reset() {
        for (auto &r : rods) while (!r.empty()) r.pop();
        for (i32 d = NUM_DISKS; d >= 1; d--)
            rods[0].push(d);
    }
    static i32 disk_y(i32 stack_index) {
        return BASE_Y - DISK_H - stack_index * DISK_H;
    }
};

struct AnimDisk {
    i32  disk;
    f32  x, y;
    f32  tx, ty;
    f32  sx, sy;
    i32  step, total;
    bool active;
 
    void start(f32 fx, f32 fy, f32 ttx, f32 tty, i32 steps) {
        sx = fx; sy = fy;
        tx = ttx; ty = tty;
        x = fx; y = fy;
        step = 0; total = steps;
        active = true;
    }
    bool tick() {
        if (!active) return false;
        step++;
        f32 t  = (f32)step / total;
        f32 et = t < 0.5f ? 2*t*t : -1+(4-2*t)*t;        
        x = sx + et * (tx - sx);
        f32 arc = sinf(t * (f32)M_PI) * 140.0f;
        y = sy + et * (ty - sy) - arc;
        if (step >= total) { x = tx; y = ty; active = false; }
        return true;
    }
};

void solve(i32 n, i32 from, i32 to, i32 aux, vector<Move> &moves);
static Color disk_color(i32 disk, i32 total);
static void fill_rect(SDL_Surface *psurface, i32 x, i32 y, i32 w, i32 h, Color c);
static void fill_rect_shaded(SDL_Surface *s, i32 x, i32 y, i32 w, i32 h, Color c);
static void render(SDL_Surface *s, const State &st, const AnimDisk &anim, i32 move_idx, i32 total_moves, const string rod_names[3]);
static constexpr Color BG = {18, 18, 28};
static constexpr Color ROD_COL = {180, 160, 120};
static constexpr Color BASE_COL = {140, 120, 90};
static constexpr Color LABEL_COL = {200, 190, 160};

//void tower_of_hanoi(i32 n, string fromRod, string toRod, string auxRod);

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "SDL init fehlgeschlagen: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow(
        "하노이탑·Tower of Hanoi",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, 0
    );
    if (!win) { SDL_Quit(); return 1; }
    SDL_Surface *surf = SDL_GetWindowSurface(win);
    if (!surf) { SDL_DestroyWindow(win); SDL_Quit(); return 1; }
    const string rod_names[3] = { "첫 번째 용", "제2의 용", "제3의 용" };
    vector<Move> moves;
    solve(NUM_DISKS, 0, 2, 1, moves);
    cout << "Moves gesamt: " << moves.size()
         << "  (2^" << NUM_DISKS << " - 1 = " << ((1 << NUM_DISKS) - 1) << ")\n";
    
    //tower_of_hanoi(n, "첫 번째 용", "제2의 용", "제3의 용");
 
    State  st;
    st.reset();
 
    AnimDisk anim{};
    anim.active = false;
 
    i32  move_idx   = 0;
    bool auto_play  = true;
    i32  auto_timer = 0;
    static constexpr i32 AUTO_PAUSE = 8; 
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_ESCAPE: running = false; break;
                        case SDLK_SPACE:
                            auto_play = !auto_play;
                            if (!auto_play && !anim.active && move_idx < (i32)moves.size()) {
                                const Move &m = moves[move_idx];
                                i32 from_cnt = (i32)st.rods[m.from].size() - 1;
                                i32 to_cnt   = (i32)st.rods[m.to].size();
                                f32 sx = (f32)ROD_X[m.from];
                                f32 sy = (f32)State::disk_y(from_cnt);
                                f32 tx = (f32)ROD_X[m.to];
                                f32 ty = (f32)State::disk_y(to_cnt);
                                st.rods[m.from].pop();
                                anim.disk = m.disk;
                                anim.start(sx, sy, tx, ty, ANIM_STEPS);
                                move_idx++;
                            }
                            break;
                        case SDLK_r:
                            st.reset();
                            moves.clear();
                            solve(NUM_DISKS, 0, 2, 1, moves);
                            move_idx = 0;
                            anim.active = false;
                            auto_play = true;
                            auto_timer = 0;
                            break;
                        default: break;
                    }
                    break;
                default: break;
            }
        }
        if (anim.active) {
            bool still_moving = anim.tick();
            if (!still_moving && move_idx > 0) {
                st.rods[moves[move_idx - 1].to].push(anim.disk);
                auto_timer = 0;
            }
        }
 
        if (auto_play && !anim.active && move_idx < (i32)moves.size()) {
            if (auto_timer++ >= AUTO_PAUSE) {
                auto_timer = 0;
                const Move &m = moves[move_idx];
                i32 from_cnt = (i32)st.rods[m.from].size() - 1;
                i32 to_cnt   = (i32)st.rods[m.to].size();
                f32 sx = (f32)ROD_X[m.from];
                f32 sy = (f32)State::disk_y(from_cnt);
                f32 tx = (f32)ROD_X[m.to];
                f32 ty = (f32)State::disk_y(to_cnt);
                    st.rods[m.from].pop();
                    anim.disk = m.disk;
                    anim.start(sx, sy, tx, ty, ANIM_STEPS);
                    move_idx++;
            }
        }
 
        render(surf, st, anim, move_idx, (i32)moves.size(), rod_names);
        SDL_UpdateWindowSurface(win);
        SDL_Delay(FRAME_DELAY);
    }
 
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
 
/*
void tower_of_hanoi(i32 n, string fromRod, string toRod, string auxRod) {
    if (n == 0) return;
    tower_of_hanoi(n - 1, fromRod, auxRod, auxRod);
    cout << "Disk " << n << "moved from " << fromRod << " to " << toRod << "\n";
    tower_of_hanoi(n - 1, auxRod, toRod, fromRod);
}
*/
static Color disk_color(i32 disk, i32 total) {
    f32 t = (f32)(disk - 1) / (f32)(total - 1);
    Color a = {80, 140, 220};
    Color b = {230, 160, 40};
    return {
        (u8)(a.r + t * (b.r - a.r)),
        (u8)(a.g + t * (b.g - a.g)),
        (u8)(a.b + t * (b.b - a.b))
    };
}

void solve(i32 n, i32 from, i32 to, i32 aux, vector<Move> &moves) {
    if (n == 0) return; 
    solve(n - 1, from, aux, to, moves);
    moves.push_back({from, to, n});
    solve(n - 1, aux, to, from, moves);
}

static void fill_rect(SDL_Surface *psurface, i32 x, i32 y, i32 w, i32 h, Color c) {
    SDL_Rect r = {x, y, w, h};
    SDL_FillRect(psurface, &r, SDL_MapRGB(psurface->format, c.r, c.g, c.b));
}

static void fill_rect_shaded(SDL_Surface *s, i32 x, i32 y, i32 w, i32 h, Color c) {
    fill_rect(s, x + 3, y + 3, w, h, { (Uint8)(c.r/3), (Uint8)(c.g/3), (Uint8)(c.b/3) });
    fill_rect(s, x, y, w, h, c);
}


static void draw_disk(SDL_Surface *s, i32 disk, i32 cx, i32 cy) {
    i32 w = DISK_W_MIN + (disk - 1) * (DISK_W_MAX - DISK_W_MIN) / (NUM_DISKS - 1);
    Color c = disk_color(disk, NUM_DISKS);
    fill_rect_shaded(s, cx - w/2, cy, w, DISK_H, c);
    fill_rect(s, cx - w/2 + 2, cy + 1, w - 4, 4, { (u8)min(255, c.r+60), (u8)min(255, c.g+60), (u8)min(255, c.b+60) });
}
 
static void render(SDL_Surface *s, const State &st,
                   const AnimDisk &anim, i32 move_idx, i32 total_moves,
                   const string rod_names[3]) {
    fill_rect(s, 0, 0, WIDTH, HEIGHT, BG);
 
    fill_rect_shaded(s, 40, BASE_Y, WIDTH - 80, BASE_H, BASE_COL);
 
    for (i32 r = 0; r < NUM_RODS; r++) {
        fill_rect_shaded(s, ROD_X[r] - ROD_W/2, ROD_TOP_Y, ROD_W, ROD_H, ROD_COL);
 
        vector<i32> disks;
        stack<i32> tmp = st.rods[r];
        while (!tmp.empty()) { disks.push_back(tmp.top()); tmp.pop(); }
        reverse(disks.begin(), disks.end());
 
        for (i32 i = 0; i < (i32)disks.size(); i++) {
            i32 disk = disks[i];
            if (anim.active && anim.disk == disk) continue;
            i32 cy = State::disk_y(i);
            draw_disk(s, disk, ROD_X[r], cy);
        }
    }
 
    if (anim.active) {
        draw_disk(s, anim.disk, (i32)anim.x, (i32)anim.y);
    }
 
    if (total_moves > 0) {
        i32 bar_w = (i32)((f32)move_idx / total_moves * (WIDTH - 80));
        fill_rect(s, 40, HEIGHT - 30, WIDTH - 80, 6, { 40, 40, 60 });
        fill_rect(s, 40, HEIGHT - 30, bar_w, 6, { 100, 180, 255 });
    }
}
 
