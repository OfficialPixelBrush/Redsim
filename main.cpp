#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

#define WORLD_WIDTH  16
#define WORLD_HEIGHT 8

int ticksSinceStart = 0;

int64_t PosToHash(int32_t x, int32_t y) {
    return ((int64_t)y << 32 | x);
}

std::pair<int32_t,int32_t> HashToPos(int64_t hash) {
    return std::pair(hash&0xFFFFFFFF, hash >> 32);
}

std::vector<std::pair<int64_t,int64_t>> tickSchedule;

enum TileId {
    EMPTY,
    DUST,
    TORCH,
    TORCH_N,
    TORCH_S,
    TORCH_E,
    TORCH_W,
    REPEATER_N,
    REPEATER_S,
    REPEATER_E,
    REPEATER_W,
    LEVER,
    SOLID
};

typedef struct Tile Tile;

#define OFF 0
#define ON 15

struct Tile {
    int8_t id = EMPTY;
    int8_t level = 0;
    bool ticked = false;
};

struct Tile world [WORLD_HEIGHT][WORLD_WIDTH];

#define DUST_CROSS "┼"
#define DUST_HORIZONTAL "─"
#define DUST_VERTICAL "│"
#define DUST_BEND_NE "└"
#define DUST_BEND_NW "┘"
#define DUST_BEND_SE "┌"
#define DUST_BEND_SW "┐"

#define CHARACTER_TORCH_C "🕯"
#define CHARACTER_TORCH_N "↑"
#define CHARACTER_TORCH_S "↓"
#define CHARACTER_TORCH_E "→"
#define CHARACTER_TORCH_W "←"

#define CHARACTER_REPEATER_N "⇑"
#define CHARACTER_REPEATER_S "⇓"
#define CHARACTER_REPEATER_E "⇒"
#define CHARACTER_REPEATER_W "⇐"

#define CHARACTER_LEVER_OFF "\\"
#define CHARACTER_LEVER_ON "/"

#define CHARACTER_BLOCK "█"

void ResetWorld() {
    // Init Empty
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            world[y][x] = Tile();
        }
    }
}

int8_t GetLevel(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return 0;
    }
    if (world[y][x].id == EMPTY) {
        return 0;
    }
    return world[y][x].level;
}

void SetLevel(int x, int y, int8_t level) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }
    if (world[y][x].id == EMPTY) {
        return;
    }
    world[y][x].level = level;
}

int8_t GetBlock(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return 0;
    }

    return world[y][x].id;
}

void PrintDust(int x, int y) {
    int8_t north = GetBlock(x  ,y+1);
    int8_t south = GetBlock(x  ,y-1);
    int8_t east  = GetBlock(x+1,y  );
    int8_t west  = GetBlock(x-1,y  );
    
    if ((north || south) && (!east && ! west)) {
        std::cout << DUST_VERTICAL;
    } else if ((!north && !south) && (east && west)) {
        std::cout << DUST_HORIZONTAL;
    } else {
        std::cout << DUST_CROSS;
    }
}

void PrintBlock(int x, int y, Tile entry) {
    int color = int((entry.level)*16);
    // Display power level
    std::cout << "\e[38;2;" << color << ";" << 0 << ";" << 0 << ";48;5;235m";

    // Print Character
    switch(entry.id) {
        case EMPTY:
            std::cout << ".";
            break;
        case DUST:
            PrintDust(x,y);
            break;
        case TORCH:
            std::cout << CHARACTER_TORCH_C;
            break;
        case REPEATER_N:
            std::cout << CHARACTER_REPEATER_N;
            break;
        case REPEATER_S:
            std::cout << CHARACTER_REPEATER_S;
            break;
        case REPEATER_E:
            std::cout << CHARACTER_REPEATER_E;
            break;
        case REPEATER_W:
            std::cout << CHARACTER_REPEATER_W;
            break;
        case LEVER:
            std::cout << CHARACTER_LEVER_OFF;
            break;
        case SOLID:
            std::cout << CHARACTER_BLOCK;
            break;
        default:
            std::cout << "x";
            break;
    }
    std::cout << "\e[0m";
}

void ClearAndHome() {
    std::cout << "\e[2J";
}

void ShowWorld() {
    ClearAndHome();
    // Init Empty
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            world[y][x].ticked = false;
            PrintBlock(x,y,world[y][x]);
        }
        std::cout << std::endl;
    }
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
}

int8_t CheckForMostPower(int x, int y) {
    int8_t north = GetLevel(x, y + 1);
    int8_t south = GetLevel(x, y - 1);
    int8_t east  = GetLevel(x + 1, y);
    int8_t west  = GetLevel(x - 1, y);

    // Step-by-step max calculation (safe for int8_t)
    int8_t max_neighbor = std::max(north, std::max(south, std::max(east, west))) - 1;

    // Ensure it doesn’t go below zero
    max_neighbor = std::max(max_neighbor, (int8_t)0);

    // Compare with current block
    return std::max(GetLevel(x, y), max_neighbor);
}

void ScheduleTick(int32_t x, int32_t y, int64_t tick) {
    tickSchedule.push_back(std::pair(PosToHash(x,y),tick));
}

void TickBlock(int x, int y, bool scheduled = false) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }
    if (world[y][x].ticked || (world[y][x].id == EMPTY)) {
        return;
    }

    switch (GetBlock(x,y)) {
        case DUST:
            world[y][x].level = CheckForMostPower(x,y);
            break;
        case REPEATER_N:
            if (GetLevel(x,y+1) && scheduled) {
                SetLevel(x,y-1,ON);
            }
            ScheduleTick(x,y,ticksSinceStart+1);
            break;
        case REPEATER_S:
            if (GetLevel(x,y-1) && scheduled) {
                SetLevel(x,y+1,ON);
            }
            ScheduleTick(x,y,ticksSinceStart+1);
        case REPEATER_E:
            if (GetLevel(x-1,y) && scheduled) {
                SetLevel(x+1,y,ON);
            }
            ScheduleTick(x,y,ticksSinceStart+1);
            break;
        case REPEATER_W:
            if (GetLevel(x+1,y) && scheduled) {
                SetLevel(x-1,y,ON);
            }
            ScheduleTick(x,y,ticksSinceStart+1);
            break;
    }
    world[y][x].ticked = true;

    // Tick surrounding
    TickBlock(x+1,y  );
    TickBlock(x-1,y  );
    TickBlock(x  ,y+1);
    TickBlock(x  ,y-1);
    return;
}

void HandleScheduledTicks() {
    std::vector<std::pair<int64_t, int64_t>> toProcess = tickSchedule;  // Copy scheduled ticks
    tickSchedule.clear();  // Clear original schedule before processing

    for (const auto& entry : toProcess) {
        if (entry.second == ticksSinceStart) {
            auto pair = HashToPos(entry.first);
            TickBlock(pair.first, pair.second, true);
        } else {
            tickSchedule.push_back(entry);  // Re-add if not processed
        }
    }
}

void PlaceBlock(int x, int y, int8_t block, int8_t level = 0) {
    HandleScheduledTicks();
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }
    world[y][x] = Tile{
        block,
        level,
        false
    };
    TickBlock(x,y);
    ShowWorld();
    ticksSinceStart++;
}

int main() {
    ResetWorld();
    PlaceBlock(1,3,DUST);
    PlaceBlock(1,4,DUST);
    PlaceBlock(1,5,DUST);
    for (int i = 1; i < 16; i++) {
        PlaceBlock(i,2,DUST);
    }
    PlaceBlock(1,1,TORCH,15);
    PlaceBlock(10,4,DUST);
    PlaceBlock(10,5,DUST);
    PlaceBlock(10,6,DUST);
    PlaceBlock(10,7,SOLID);
    PlaceBlock(10,3,REPEATER_S);

    PlaceBlock(6,6,DUST);
    PlaceBlock(7,6,REPEATER_W);
    PlaceBlock(8,6,DUST);

    PlaceBlock(6,5,DUST);
    PlaceBlock(7,5,REPEATER_E);
    PlaceBlock(8,5,DUST);

    PlaceBlock(9,5,DUST);
    PlaceBlock(9,5,EMPTY);
    while(true) {
        PlaceBlock(0,0,EMPTY);
    }
    return 0;
}