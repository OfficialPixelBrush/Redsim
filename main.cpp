#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#define WORLD_WIDTH  16
#define WORLD_HEIGHT 8

int8_t world [WORLD_HEIGHT][WORLD_WIDTH];

// Bit 0-3 Strength
// Bit 4 Dust
// Bit 5 Torch
// Bit 6 Repeater
// Bit 7 Lever
#define EMPTY    0b00000000
#define DUST     0b00010000
#define TORCH    0b00100000 
#define REPEATER 0b00110000
#define LEVER    0b01000000
#define SOLID    0b01010000

#define BLOCK    0b01110000
#define LEVEL    0b00001111

#define TICKED   0b10000000

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
            world[y][x] = 0;
        }
    }
}

int8_t GetLevel(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return 0;
    }
    if ((world[y][x] & BLOCK) == EMPTY) {
        return 0;
    }
    return world[y][x] & LEVEL;
}

int8_t GetBlock(int x, int y) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return 0;
    }

    return world[y][x] & BLOCK;
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

void PrintBlock(int x, int y, int8_t entry) {
    int color = int((entry&LEVEL)*16);
    // Display power level
    std::cout << "\e[38;2;" << color << ";" << 0 << ";" << 0 << ";48;5;235m";

    // Print Character
    switch(entry & BLOCK) {
        case EMPTY:
            std::cout << ".";
            break;
        case DUST:
            PrintDust(x,y);
            break;
        case TORCH:
            std::cout << CHARACTER_TORCH_C;
            break;
        case REPEATER:
            std::cout << CHARACTER_REPEATER_E;
            break;
        case LEVER:
            std::cout << CHARACTER_LEVER_OFF;
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
            world[y][x] = world[y][x] & ~TICKED;
            PrintBlock(x,y,world[y][x]);
        }
        std::cout << std::endl;
    }
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

void TickBlock(int x, int y, int8_t level) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }
    if (world[y][x] & TICKED || (world[y][x] & BLOCK) == EMPTY) {
        return;
    }

    world[y][x] = (world[y][x] & BLOCK) | CheckForMostPower(x,y);
    world[y][x] = world[y][x] | TICKED;

    // Tick surrounding
    TickBlock(x+1,y  ,level-1);
    TickBlock(x-1,y  ,level-1);
    TickBlock(x  ,y+1,level-1);
    TickBlock(x  ,y-1,level-1);
    return;
}

void PlaceBlock(int x, int y, int8_t block, int8_t level = 0) {
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
        return;
    }
    world[y][x] = block | level;
    TickBlock(x,y,level);
}

int main() {
    ResetWorld();
    PlaceBlock(1,3,DUST);
    PlaceBlock(1,4,DUST);
    PlaceBlock(1,5,DUST);
    ShowWorld();
    int i = 1;
    while(true) {
        PlaceBlock(i,2,DUST);
        ShowWorld();
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
        if (i < 15) {
            i++;
        }
        if (i == 15) {
            PlaceBlock(1,1,TORCH,15);
        }
    }
    return 0;
}