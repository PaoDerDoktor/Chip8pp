#pragma once

#include <array>
#include <stack>
#include <spdlog/spdlog.h>
#include <string>
#include <cstdint>
#include <random>

#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

bool init_emu();
bool terminate_emu();
std::string get_bin_representation(uint original);
std::string get_hex_representation(uint original);

class Chip8 {
    private: // Private fields
        std::string               name;              ///< Name/identifir (for logging)
        std::array<uint8_t, 4096> ram;               ///< 4KB of RAM
        std::array<uint8_t, 16>   variableRegisters; ///< V0-VF Variable registers 

        std::mt19937                            randomEngine;       ///< Random Engine
        std::uniform_int_distribution<uint32_t> randomDistribution; ///< Random distribution. Should be 0-255.

        uint16_t             pc;            ///< Program Counter
        uint16_t             indexRegister; ///< Index Register
        std::stack<uint16_t> addressStack;  ///< Stack of call addresses
        uint8_t              delayTimer;    ///< 60Hz - delay timer
        uint8_t              soundTimer;    ///< Sound timer

        GLFWwindow                           *display;      ///< Window where to display
        std::array<std::array<bool, 32>, 64>  displayState; ///< 2D array representing the image to render

        uint16_t rawInstruction; ///< Raw 16-bit instruction to be decoded

        // Decoded instuction (every component is computed rregardless of the opcode)
        uint8_t  opcode;           ///< The 4-bits opcode to be executed
        uint8_t  firstRegister;    ///< The 4-bits first register index of the instruction
        uint8_t  secondRegister;   ///< The 4-bits second register index of the instruction
        uint8_t  spriteSize;       ///< The 4-bits size of the sprite to render
        uint8_t  immediateValue;   ///< The 8-bits immediate value of the instruction
        uint16_t immediateAddress; ///< The 12-bits immediate address of the instruction

        // OpenGL-rendering-related fields
        GLuint    vaoAddress;                      ///< Address of the pixel VAO
        GLuint    programAddress;                  ///< Address of the main pixel rendering program
        glm::vec4 pixelColor;                      ///< chosen pixel color
        GLint     enabledColorUniformLocation;     ///< Location of the enabled color uniform
        GLint     projectionMatrixUniformLocation; ///< Location of the projection matrix uniform
        GLint     modelMatrixUniformLocation;      ///< Location of the model matrix uniform

    public:  // Public functions
        Chip8(const std::string &name);
        
        void load_program(const std::string &fileName);
        void run();

        // Getters
        std::array<std::array<bool, 32>, 64> get_display_state();
        GLint get_model_matrix_uniform_location()      const;
        GLint get_projection_matrix_uniform_location() const;
        GLint get_enabled_color_uniform_location()     const;
        GLint get_program_address()                    const;

    private: // Private functions
        // I/O
        char read(const uint16_t &address);
        void write(const uint16_t &address, const uint8_t &value);

        // Fetch-Decode-Execute cycle
        void fetch();
        void decode();
        void execute();

        // Chip8 operations
        void execute_machine_routine();
        void clear_screen();
        void jump();
        void call_subroutine();
        void exit_subroutine();
        void skip_if_value();
        void skip_if_not_value();
        void skip_if_equals_register();
        void skip_if_not_equals_register();
        void set_var_register();
        void add_var_register();
        void set_from_other_register();
        void bin_or();
        void bin_and();
        void bin_xor();
        void add_from_other_register();
        void substract();
        void substract_reverse();
        void bin_shift_left();
        void bin_shift_right();
        void set_index_register();
        void jump_with_offset();
        void random();
        void draw();
        void skip_if_key();
        void skip_if_not_key();
        void set_reg_to_delay_timer();
        void set_delay_timer_to_reg();
        void set_sound_timer_to_reg();
        void add_to_index_register();
        void get_key();
        void set_index_reg_to_character();
        void decimal_conversion();
        void memory_store();
        void memory_load();


        // Meta-operations
        void update_display();

    private: // Private static functions
        static void glfw_error_callback(int error, const char *description);
        static void glfw_frame_size_callback(GLFWwindow *window, int width, int height);
        static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

};
