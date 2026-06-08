# CHIP-8 / SuperCHIP (SCHIP) Emulator

A portable CHIP-8 and SuperCHIP (SCHIP) emulator written in C++ and compiled to WebAssembly (WASM) using Emscripten.

---

## 🛠️ Architecture Overview

The emulator replicates a standard 8-bit virtual machine architecture consisting of the following modules:

1. **CPU & Interpreter Core (`cpu.cpp`)**:
   - **Registers**: Features 16 general-purpose 8-bit registers ($V_0$ to $V_F$). Register $V_F$ doubles as a carry/collision flag.
   - **Index Register ($I$)**: A 16-bit register used to point to memory addresses.
   - **Memory Map**: Simulates a 4096-byte addressable space. The first 512 bytes ($0x000$ to $0x1FF$) are reserved for font sets and system variables; program instructions load starting at address $0x200$.
   - **Program Counter ($PC$)**: A 16-bit register tracking the address of the currently executing instruction.
   - **Stack & SP**: A 32-level stack storing return addresses ($16$-bit) during subroutine calls, managed by a Stack Pointer ($sp$).
   - **Timers**: Two 8-bit registers decrementing at $60\text{Hz}$—the *Delay Timer* (tracks timing delay) and *Sound Timer* (beeps when greater than 0).

2. **Graphics & Framebuffers**:
   - **Dual Framebuffers**: Simulates both standard CHIP-8 ($64 \times 32$ pixels) and SuperCHIP extended resolution ($128 \times 64$ pixels) framebuffers.
   - **SDL2 Renderer (`main.cpp`)**: A dynamic SDL2 loop updates a streaming texture. The texture is normalized to $128 \times 64$ pixels, scaling standard resolution frames by $2\times$ horizontally and vertically to maintain visual consistency without changing window bounds.

3. **Input Handling**:
   - A 16-key hex keypad ($0$ through $F$) mapped directly to standard keyboard keys.

---

## ✨ Features & Additions

This version includes several major fixes and feature improvements over standard baseline emulators:

* **Pixel Clipping (Original VIP Quirk)**: Sprites drawn off the bottom or right edges of the screen are correctly **clipped** instead of wrapping around. This fixes collision and rendering failures in classic games like *Blitz* where bottom-drawn structures would wrap to the top of the screen.
* **Jump Instruction Pipeline Corrections**: Fixed a bug where `1NNN` (Jump) and `BNNN` (Jump + $V_0$) were calling `increment_pc()` after assigning the target address, resulting in the interpreter skipping the first target instruction.
* **EX9E & EXA1 Key Skip Logic**: Corrected the opcode checks to mask `current_opcode` (`& 0x00FF`) rather than performing a raw comparison. Also implemented proper program counter progression on fallthrough to prevent infinite execution hangs.
* **RPL Flags Buffer Overflow Protection**: Increased the `rpl_user_flags` array size to `16` to prevent out-of-bounds register state writes ($V_0 - V_F$) from corrupting other emulator states.
* **WebAssembly Portability**: Built-in support for Emscripten's tick callback system, letting the engine run asynchronously at $60\text{Hz}$ in modern web browsers without blocking the event loop. Exposes raw memory write callbacks (`load_rom_data`) for web integration.

---

## 🚀 How to Run

### 1. Compile and Run Locally (Desktop)

Make sure you have `g++` and **SDL2** installed on your system. Run:

```bash
# Compile the native executable
make game

# Run a ROM (e.g. Blitz)
./play "Blitz [David Winter].ch8"
```

### 2. Compile to WebAssembly (WASM)

To build the project for WebAssembly, you need the Emscripten SDK (`emsdk`). Run:

```bash
# Compile to WebAssembly (play.js and play.wasm)
make wasm
```

This outputs `play.js` and `play.wasm` directly into the project directory, which are loaded automatically by `index.html`. You can host `index.html` on **GitHub Pages** to play CHIP-8 games right in your browser with drag-and-drop support.

---

## 📚 Credits & Resources

This implementation drew architectural guidance and specifications from these excellent sources:

1. **[Cowgod's CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8tech10.html)** by Thomas P. Greene - *The definitive standard for opcode operations and memory layouts.*
2. **[How to write a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/)** by Tobias V. Langhoff - *Detailed breakdown of interpreter timing, loop patterns, and quirks.*
3. **[How to Write an Emulator (CHIP-8 Interpreter)](https://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/)** by Laurence Muller - *Practical implementation blueprints for standard virtual machinery and SDL pipelines.*
4. **CHIP-48 & SuperCHIP Specifications** by Erik Bryntse - *Extended instruction documentation for high-resolution graphics and scrolling.*
