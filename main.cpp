#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include "include/SDL2/SDL.h"
#include <unordered_map>
#include "cpu.cpp"
#include <vector>


using namespace std;

class Chip8Emulator 
{
public:
    Chip8Emulator() 
    {
        cout << "CHIP8 Started!" << endl;
        cout << "Initializing SDL!" << endl;

        if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
            cerr << "SDL Initialization Failed!" << endl;
            exit(EXIT_FAILURE); // Handle the failure using exit
        }


        window = SDL_CreateWindow("CHIP8 Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_SHOWN);
        if(window == nullptr)
        {
            cerr << "SDL Window Initialization Failed!" << endl;
            exit(EXIT_FAILURE); // Handle the failure using exit
        }


        renderer = SDL_CreateRenderer(window, -1, 0);

        if (renderer == nullptr) {
            const char* err = SDL_GetError();
            cerr << err << endl;
            cerr << "SDL Renderer Initialization Failed!" << endl;
            exit(EXIT_FAILURE); // Handle the failure using exit
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 128, 64);
        if (texture == nullptr) {
            cerr << "SDL Texture Creation Failed!" << endl;
            exit(EXIT_FAILURE); // Handle the failure using exit
        }
    }

    ~Chip8Emulator() 
    {
        //SDL_DestroyTexture(texture);
		//SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        window = nullptr;
        cout << "Quitting SDL!" << endl;
        SDL_Quit();
        cout << "CHIP8 Exiting! 😵" << endl;
    }

    void clear_window()
    {
        SDL_RenderClear(renderer);
    }

    void present_render()
    {
        SDL_RenderPresent(renderer);
    }

   
    SDL_Texture* getSDL_Texture ()
    {
        return texture;
    }

     SDL_Renderer* getSDL_Renderer ()
    {
        return renderer;
    }

     SDL_Window* getSDL_Window ()
    {
        return window;
    }


private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    

};

void loadROM(const char* filename, Chip8 &cpu)  // Implement file opener
{
    std::ifstream rom(filename, std::ios::binary);

    //auto fileDeleter = [](std::ifstream* file) { file->close(); };
    // Updated lambda function usage
    //std::unique_ptr<std::ifstream, decltype(fileDeleter)> inputFilePtr(&rom, fileDeleter);

    if (rom.is_open()) {
        rom.seekg(0, std::ios::end);
        std::streampos size = rom.tellg();
        rom.seekg(0, std::ios::beg);

        cout << "Loading ROM: " << filename << endl;    
        if (size > 0 && size <= 0xFFFE00) {
            // Read the ROM directly into Chip-8 memory starting from 0x200
            rom.read(reinterpret_cast<char*>(&cpu.memory[0x200]), size);
        }
        // for (size_t i = 0; i < size; ++i) {
        //     cpu.memory[i + 0x200] = rom.get();
        // }

        rom.close();
        cout << "Rom Loaded" << endl;

    }
}

void buildTexture(Chip8Emulator &emulator, Chip8 &cpu)
{
    uint32_t* bytes = nullptr;
    int pitch = 0;

    SDL_LockTexture(emulator.getSDL_Texture(), nullptr, reinterpret_cast<void**>(&bytes), &pitch);

    if(!cpu.extendedScreenMode)
    {
        for (size_t y = 0; y < 64; ++y) {
            for (size_t x = 0; x < 128; ++x) {
                bytes[y * 128 + x] = (cpu.graphics[(y / 2) * 64 + (x / 2)] == 1) ? 0xFFFFFFFF : 0x000000FF;
            }
        }
    }
    else
    {
        for (size_t y = 0; y < 64; ++y) {
            for (size_t x = 0; x < 128; ++x) {
                bytes[y * 128 + x] = (cpu.graphics_extended[y * 128 + x] == 1) ? 0xFFFFFFFF : 0x000000FF;
            }
        }
    }

    SDL_UnlockTexture(emulator.getSDL_Texture());
}

const array<int, 16> keymap = {{
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V
}};

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Globals for WebAssembly / main loop callback
Chip8Emulator* global_emulator = nullptr;
Chip8* global_cpu = nullptr;
bool global_running = true;
chrono::milliseconds global_duration(16);

void run_loop_step()
{
    if (global_cpu == nullptr || global_emulator == nullptr)
        return;

    auto x = global_cpu->program_counter; // debugging

    // Emulation Cycle
    SDL_Event event;
    global_cpu->cycle();

    char hex_string[20];
    if(x != global_cpu->program_counter)
    {
        sprintf(hex_string, "%X", global_cpu->current_opcode); //convert number to hex
        cout << "debugging message: 0x" << hex_string << endl; 
    }
    
    while( SDL_PollEvent(&event) > 0 )
    {
        switch(event.type)
        {
            case SDL_QUIT:
            {
                global_running = false;
                break;
            }
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) 
                {
                    global_running = false;
                }
                for (int i = 0; i < 16; ++i) {
                    if (event.key.keysym.scancode == keymap[i]) {
                        global_cpu->keys[i] = 1;
                    }
                }
                break;
            case SDL_KEYUP:
                for (int i = 0; i < 16; ++i) {
                    if (event.key.keysym.scancode == keymap[i]) {
                        global_cpu->keys[i] = 0;
                    }
                }
                break;
            default:
                break;
        }   
    }

    global_emulator->clear_window();
    buildTexture(*global_emulator, *global_cpu);
    SDL_Rect dest = {0, 0, 640, 320};
    SDL_RenderCopy(global_emulator->getSDL_Renderer(), global_emulator->getSDL_Texture() , nullptr, &dest);
    global_emulator->present_render();
}

extern "C" {
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
#endif
    void load_rom_data(const uint8_t* data, int size) {
        if (global_cpu != nullptr) {
            global_cpu->init();
            for (int i = 0; i < size && (0x200 + i) < 4096; ++i) {
                global_cpu->memory[0x200 + i] = data[i];
            }
            std::cout << "Loaded ROM data directly: " << size << " bytes." << std::endl;
        }
    }
}



int main(int argc, char* argv[]) 
{
    const char* rom_path = "test_opcode.ch8";
    if (argc > 1) {
        rom_path = argv[1];
    }


    Chip8Emulator emulator;
    Chip8 cpu;

    //cout << __cplusplus << endl;
    bool running = true;
    int duration_ms = 16;
    auto duration = chrono::milliseconds(duration_ms);

    
    cpu.init();
    cout << "init functions ran" << endl;

    for(int i = 0; i < 8; i++) 
        cpu.rpl_user_flags[i] = rand() & 0x3F;


#ifndef __EMSCRIPTEN__
    loadROM(rom_path, cpu);
#endif

    // Setup global pointers for Emscripten loop callback
    global_emulator = &emulator;
    global_cpu = &cpu;
    global_running = true;
    global_duration = duration;

    cout << "Emulator cycle begins" << endl;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(run_loop_step, 0, 1);
#else
    while(global_running)
    {
        run_loop_step();
        this_thread::sleep_for(global_duration);
    }
#endif

    return 0;

}
