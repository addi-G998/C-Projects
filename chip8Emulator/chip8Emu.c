#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define Kilobytes(n) ((n) * 1024)

typedef unsigned char ubchar_8;

typedef struct {
  uint8_t memory[Kilobytes(4)];
  // Registers V0 - VF
  uint8_t V[16];
  uint16_t stack[16];

  uint16_t I;
  uint16_t PC;
  uint8_t SP;

  uint8_t sound_reg;
  uint8_t delay_reg;

  uint8_t framebuffer[64 * 32];
  uint8_t keypad[16];
} chip_8_processor;

void execute(uint16_t opcode, chip_8_processor *chp) {
  uint8_t op = (opcode & 0xF000) >> 12;
  uint8_t vx = (opcode & 0x0F00) >> 8;
  uint8_t vy = (opcode & 0x00F0) >> 4;
  uint8_t nb = (opcode & 0x000F);

  switch (op) {
  case 0x0: {
    // SYS addr - Jump to machine code routine at nnn
    uint8_t lb = (opcode & 0x0FF);
    if (lb == 0xE0) { // CLS
      memset(chp->framebuffer, 0, sizeof(chp->framebuffer));
    } else if (lb == 0xEE) { // RET
      chp->SP--;
      chp->PC = chp->stack[chp->SP];

    } else {
    } // 0nnn
    break;
  }
  case 0x1: {
    // JUMP
    uint16_t nnn = (opcode & 0x0FFF);
    chp->PC = nnn;
    break;
  }
  case 0x2: {
    // CALL ADDR
    uint16_t nnn = (opcode & 0x0FFF);
    chp->stack[chp->SP] = chp->PC;
    chp->SP++;
    chp->PC = nnn;
    break;
  }
  case 0x3: {
    // SKIP IF EQUAL
    uint8_t kk = (opcode & 0x0FF);
    if (chp->V[vx] == kk) {
      chp->PC += 2;
    }
    break;
  }

  case 0x4: {
    // SKIP IF NOT EQUAL
    uint8_t kk = (opcode & 0x0FF);
    if (chp->V[vx] != kk) {
      chp->PC += 2;
    }
    break;
  }
  case 0x5: { // Skip Instruction
    if (chp->V[vx] == chp->V[vy]) {
      chp->PC += 2;
    }
    break;
  }
  case 0x6: {
    uint8_t kk = (opcode & 0x00FF);
    chp->V[vx] = kk;
    break;
  }
  case 0x7: {
    // ADD
    uint8_t kk = (opcode & 0x0FF);
    chp->V[vx] += kk;
    break;
  }
  case 0x8: { // Mathmatical operations
    uint8_t nb = (opcode & 0x0F);
    if (nb == 0x0) {
      chp->V[vx] = chp->V[vy];
      break;
    } else if (nb == 0x1) {
      chp->V[vx] = (chp->V[vx] | chp->V[vy]);
      break;
    } else if (nb == 0x2) {
      chp->V[vx] = (chp->V[vx] & chp->V[vy]);
      break;
    } else if (nb == 0x3) {
      chp->V[vx] = (chp->V[vx] ^ chp->V[vy]);
      break;
    } else if (nb == 0x4) {
      uint16_t sum = chp->V[vx] + chp->V[vy];
      chp->V[0xF] = sum > 0xFF ? 1 : 0; // carry if overflows 8 bits
      chp->V[vx] = sum & 0xFF;          // keep lower 8 bits
      break;
    } else if (nb == 0x5) {
      uint8_t flag = chp->V[vx] > chp->V[vy] ? 1 : 0;
      chp->V[vx] = (chp->V[vx] - chp->V[vy]);
      chp->V[0xF] = flag;
      break;
    } else if (nb == 0x6) {
      uint8_t flag = chp->V[vx] & 0x1; // save LSB first
      chp->V[vx] = chp->V[vx] >> 1;    // shift right by 1
      chp->V[0xF] = flag;              // write VF last
      break;
    } else if (nb == 0x7) {
      uint8_t flag = (chp->V[vx] < chp->V[vy] ? 1 : 0);
      chp->V[vx] = (chp->V[vy] - chp->V[vx]);
      chp->V[0xF] = flag;
      break;
    } else if (nb == 0xE) {
      uint8_t flag = (chp->V[vx] >> 7) & 0x1;
      chp->V[vx] = chp->V[vx] << 1;
      chp->V[0xF] = flag;
      break;
    }
    break;
  }
  case 0x9: {
    if (chp->V[vx] != chp->V[vy]) {
      chp->PC += 2;
    }
    break;
  }
  case 0xA: {
    chp->I = (opcode & 0xFFF);
    break;
  }
  case 0xB: {
    uint16_t nnn = (opcode & 0x0FFF);
    chp->PC = nnn + chp->V[0x0];
    break;
  }
  case 0xC: {
    uint8_t n = 255;
    uint8_t kk = (opcode & 0x0FF);
    uint8_t r = rand() % (n + 1);
    chp->V[vx] = (kk & r);
    break;
  }
  case 0xD: { // Draw n-byte sprite
    for (int i = 0; i < nb; i++) {
      uint8_t row = chp->memory[chp->I + i]; // read row from memory
      for (int j = 0; j < 8; j++) {
        if (row & (0x80 >> j)) {             // check each bit
          uint8_t x = (chp->V[vx] + j) % 64; // get x and y coordinates
          uint8_t y = (chp->V[vy] + i) % 32;
          if (chp->framebuffer[y * 64 + x] ==
              1) { // translate 1-D Array to 2-D Array
            chp->V[0xF] = 1;
          }
          chp->framebuffer[y * 64 + x] ^= 1;
        }
      }
    }
    break;
  }
  case 0xE: {
    uint8_t lb = (opcode & 0x00FF);
    uint8_t key = chp->V[vx];
    if (lb == 0x9E) {
      if (chp->keypad[key] == 1) {
        chp->PC += 2;
      }

    } else if (lb == 0xA1) {
      if (chp->keypad[key] == 0) {
        chp->PC += 2;
      }
    }
    break;
  }
  case 0xF: {
    uint8_t lb = (opcode & 0xFF);
    if (lb == 0x07) {
      chp->V[vx] = chp->delay_reg;
    } else if (lb == 0x0A) {
      bool key_found = false;
      for (int k = 0; k < 16; k++) {
        if (chp->keypad[k]) {
          chp->V[vx] = k;
          key_found = true;
          break;
        }
      }
      if (!key_found)
        chp->PC -= 2; // ← inside the else if, not floating outside
    } else if (lb == 0x15) {
      chp->delay_reg = chp->V[vx];
    } else if (lb == 0x18) {
      chp->sound_reg = chp->V[vx];
    } else if (lb == 0x1E) {
      chp->I += chp->V[vx];
    } else if (lb == 0x29) {
      chp->I = chp->V[vx] * 5;
    } else if (lb == 0x33) {
      chp->memory[chp->I] = chp->V[vx] / 100;
      chp->memory[chp->I + 1] = (chp->V[vx] / 10) % 10;
      chp->memory[chp->I + 2] = chp->V[vx] % 10;
    } else if (lb == 0x55) {
      for (int i = 0; i <= vx; i++) {
        chp->memory[chp->I + i] = chp->V[i];
      }
    } else if (lb == 0x65) {
      for (int i = 0; i <= vx; i++) {
        chp->V[i] = chp->memory[chp->I + i];
      }
    }
    break; // ← one break at the end
  }

  default:
    break;
  } // Default case
}

int main(int argc, char *argv[]) {
  chip_8_processor chp = {0};
  chp.PC = 0x200;

  // open .ch8 rom file
  FILE *rom = fopen(argv[1], "rb");
  if (!rom) {
    printf("failed to open ROM\n");
    return 1;
  }
  fread(&chp.memory[0x200], 1, sizeof(chp.memory) - 0x200, rom);
  fclose(rom);
  // set text font
  uint8_t font[] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };
  memcpy(&chp.memory[0x000], font, sizeof(font));

  // 1. Initialize SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("error initializing SDL: %s\n", SDL_GetError());
    return 1;
  }

  // 2. Create the window
  SDL_Window *win = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED, 1000, 1000, 0);
  if (!win) {
    printf("Window error: %s\n", SDL_GetError());
    return 1;
  }

  // 3. Create renderer AFTER the window exists
  SDL_Renderer *renderer =
      SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

  // 4. Create the texture
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING, 64, 32);

  // main loop
  SDL_Event event;
  int running = 1;

  while (running) {
    uint32_t frame_start = SDL_GetTicks();
    // handle input
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        running = 0;
      // key mappings
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_1:
          chp.keypad[0x1] = 1;
          break;
        case SDLK_2:
          chp.keypad[0x2] = 1;
          break;
        case SDLK_3:
          chp.keypad[0x3] = 1;
          break;
        case SDLK_4:
          chp.keypad[0xC] = 1;
          break;
        case SDLK_q:
          chp.keypad[0x4] = 1;
          break;
        case SDLK_w:
          chp.keypad[0x5] = 1;
          break;
        case SDLK_e:
          chp.keypad[0x6] = 1;
          break;
        case SDLK_r:
          chp.keypad[0xD] = 1;
          break;
        case SDLK_a:
          chp.keypad[0x7] = 1;
          break;
        case SDLK_s:
          chp.keypad[0x8] = 1;
          break;
        case SDLK_d:
          chp.keypad[0x9] = 1;
          break;
        case SDLK_f:
          chp.keypad[0xE] = 1;
          break;
        case SDLK_z:
          chp.keypad[0xA] = 1;
          break;
        case SDLK_x:
          chp.keypad[0x0] = 1;
          break;
        case SDLK_c:
          chp.keypad[0xB] = 1;
          break;
        case SDLK_v:
          chp.keypad[0xF] = 1;
          break;
        }
      }
      if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_1:
          chp.keypad[0x1] = 0;
          break;
        case SDLK_2:
          chp.keypad[0x2] = 0;
          break;
        case SDLK_3:
          chp.keypad[0x3] = 0;
          break;
        case SDLK_4:
          chp.keypad[0xC] = 0;
          break;
        case SDLK_q:
          chp.keypad[0x4] = 0;
          break;
        case SDLK_w:
          chp.keypad[0x5] = 0;
          break;
        case SDLK_e:
          chp.keypad[0x6] = 0;
          break;
        case SDLK_r:
          chp.keypad[0xD] = 0;
          break;
        case SDLK_a:
          chp.keypad[0x7] = 0;
          break;
        case SDLK_s:
          chp.keypad[0x8] = 0;
          break;
        case SDLK_d:
          chp.keypad[0x9] = 0;
          break;
        case SDLK_f:
          chp.keypad[0xE] = 0;
          break;
        case SDLK_z:
          chp.keypad[0xA] = 0;
          break;
        case SDLK_x:
          chp.keypad[0x0] = 0;
          break;
        case SDLK_c:
          chp.keypad[0xB] = 0;
          break;
        case SDLK_v:
          chp.keypad[0xF] = 0;
          break;
        }
      }
    }

    // run CPU cycles
    for (int i = 0; i < 10; i++) { // ~10 opcodes per frame
      uint16_t opcode = (chp.memory[chp.PC] << 8) | chp.memory[chp.PC + 1];
      chp.PC += 2;
      execute(opcode, &chp);
    }

    // update timers at 60hz
    if (chp.delay_reg > 0)
      chp.delay_reg--;
    if (chp.sound_reg > 0)
      chp.sound_reg--;

    // render

    // 1. Create a buffer that SDL can understand (32 bits per pixel)
    uint32_t pixels[64 * 32];

    // 2. Map CHIP-8 bits to colors
    for (int i = 0; i < 64 * 32; i++) {
      // If the framebuffer byte is non-zero, set pixel to White, else Black
      pixels[i] = (chp.framebuffer[i] != 0) ? 0xFFFFFFFF : 0x00000000;
    }

    // 3. Push these pixels to the GPU texture
    SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // cap FPS
    uint32_t frame_time = SDL_GetTicks() - frame_start;
    if (frame_time < 16) {
      SDL_Delay(16 - frame_time); // sleep the remaining time
    }
  }
  // Cleanup
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
