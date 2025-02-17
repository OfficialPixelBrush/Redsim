#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#define WORLD_WIDTH  16
#define WORLD_HEIGHT 8

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

#define BLOCK    0b01110000
#define LEVEL    0b00001111

#define TICKED   0b10000000

#define CHARACTER_DUST "┼"

#define CHARACTER_TORCH "i"

#define CHARACTER_REPEATER "="

#define CHARACTER_LEVER_OFF "\\"
#define CHARACTER_LEVER_ON "/"

int8_t world [WORLD_HEIGHT][WORLD_WIDTH];

void ResetWorld() {
    // Init Empty
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            world[y][x] = 0;
        }
    }
}

void PrintBlock(int8_t entry) {
    int color = int((entry&LEVEL)*16);
    // Display power level
    std::cout << "\e[38;2;" << color << ";" << 0 << ";" << 0 << "m";

    // Print Character
    switch(entry & BLOCK) {
        case EMPTY:
            std::cout << ".";
            break;
        case DUST:
            std::cout << CHARACTER_DUST;
            break;
        case TORCH:
            std::cout << CHARACTER_TORCH;
            break;
        case REPEATER:
            std::cout << CHARACTER_REPEATER;
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
            PrintBlock(world[y][x]);
        }
        std::cout << std::endl;
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
    PlaceBlock(15,2,TORCH,15);
    ShowWorld();
    int i = 0;
    while(true) {
        PlaceBlock(i,2,DUST);
        ShowWorld();
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
        if (i < 14) {
            i++;
        }
    }
    return 0;
}