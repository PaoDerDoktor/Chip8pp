#include "Chip8.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <cmath>
#include <random>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


bool init_emu() {
    bool glfwInitialization = glfwInit();
    if (!glfwInitialization) {
        throw std::logic_error("GLFW error : Failed to initialize GLFW");
    }
    return glfwInitialization;
}

bool terminate_emu() {
    glfwTerminate();

    return true;
}

std::string get_bin_representation(uint original) {
    std::string representation;
    
    while (original > 0) {
        uint remainder = original%2;
        representation.insert(0, std::to_string(remainder));
        original /= 2;
    }

    return representation;
}

std::string get_hex_representation(uint original) {
    std::string representation;

    while (original > 0) {
        uint remainder = original%16;
        std::string toInsert;

        switch (remainder) {
            default:
            case 0:
                toInsert = "0";
                break;

            case 1:
                toInsert = "1";
                break;

            case 2:
                toInsert = "2";
                break;

            case 3:
                toInsert = "3";
                break;

            case 4:
                toInsert = "4";
                break;

            case 5:
                toInsert = "5";
                break;

            case 6:
                toInsert = "6";
                break;

            case 7:
                toInsert = "7";
                break;

            case 8:
                toInsert = "8";
                break;

            case 9:
                toInsert = "9";
                break;

            case 10:
                toInsert = "A";
                break;

            case 11:
                toInsert = "B";
                break;

            case 12:
                toInsert = "C";
                break;

            case 13:
                toInsert = "D";
                break;

            case 14:
                toInsert = "E";
                break;

            case 15:
                toInsert = "F";
                break;
        }

        representation.insert(0, toInsert);
        original /= 16;
    }

    return representation;
}


Chip8::Chip8(const std::string &name) : name(name),        pc(0),
                                        indexRegister(0),  addressStack(),
                                        delayTimer(60),    soundTimer(60),
                                        opcode(0),         firstRegister(0),
                                        secondRegister(0), spriteSize(0),
                                        immediateValue(0), immediateAddress(0),
                                        randomEngine(),    randomDistribution(0, 255) {
    // GLFW window preparation
    this->display = glfwCreateWindow(800, 400, "pico8", NULL, NULL);
    std::cout << "IF 0 CHECK IF EMU IS INIT'D : " << this->display << std::endl;

    glfwSetWindowUserPointer(this->display, (void *)this);
    glfwMakeContextCurrent(this->display);
    gladLoadGL(glfwGetProcAddress);

    glfwSetFramebufferSizeCallback(this->display, Chip8::glfw_frame_size_callback);
    glfwSetKeyCallback(this->display, Chip8::glfw_key_callback);
    glfwSetErrorCallback(Chip8::glfw_error_callback);

    // Prepare OpenGL renderer

    float squareVertices[] = {1.0f, 1.0f,    1.0f,  0.0f,    0.0f,  0.0f,    0.0f, 1.0f};
    //float squareVertices[] = {0.0f, 0.5f,    0.0f,  -0.5f,    -1.0f,  -0.5f,    -1.0f, 0.5f};
    //float squareVertices[] = {0.5f, 0.5f,    0.5f, -0.5f,   -0.5f, -0.5f,   -0.5f, 0.5f};
    unsigned int squareIndices[] = {0, 1, 3,    1, 2, 3};
    
    GLuint vboAddress;
    GLuint eboAddress;

    glGenBuffers(1, &vboAddress);
    glGenBuffers(1, &eboAddress);
    glGenVertexArrays(1, &this->vaoAddress);

    glBindVertexArray(vaoAddress);
    glBindBuffer(GL_ARRAY_BUFFER,         vboAddress);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboAddress);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // Vertices
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);

    this->pixelColor = glm::vec4(1.0, 1.0, 1.0, 1.0);

    GLuint vShaderAddress, fShaderAddress;
    vShaderAddress = glCreateShader(GL_VERTEX_SHADER);
    fShaderAddress = glCreateShader(GL_FRAGMENT_SHADER);

    std::ifstream vShaderFile("resources/shaders/grid.v.glsl");
    std::ifstream fShaderFile("resources/shaders/grid.f.glsl");

    std::string vShaderSourceCode((std::istreambuf_iterator<char>(vShaderFile)), std::istreambuf_iterator<char>());
    std::string fShaderSourceCode((std::istreambuf_iterator<char>(fShaderFile)), std::istreambuf_iterator<char>());

    vShaderFile.close();
    fShaderFile.close();

    const char *vShaderSourceRaw = vShaderSourceCode.c_str();
    const char *fShaderSourceRaw = fShaderSourceCode.c_str();

    const int vShaderSourceSize = static_cast<int>(vShaderSourceCode.length());
    const int fShaderSourceSize = static_cast<int>(fShaderSourceCode.length());

    glShaderSource(vShaderAddress, 1, &vShaderSourceRaw, &vShaderSourceSize);
    glShaderSource(fShaderAddress, 1, &fShaderSourceRaw, &fShaderSourceSize);

    glCompileShader(vShaderAddress);

    GLint vSuccess = 0;
    glGetShaderiv(vShaderAddress, GL_COMPILE_STATUS, &vSuccess);

    if (vSuccess == GL_FALSE) {
        GLint logSize = 0;
        glGetShaderiv(vShaderAddress, GL_INFO_LOG_LENGTH, &logSize);
        
        GLchar *log;
        glGetShaderInfoLog(vShaderAddress, logSize, NULL, log);

        std::cout << "SHADER ERROR: ";
        for (size_t c=0; c < logSize; ++c) {
            std::cout << log[c];
        }
        std::cout << std::endl;

        throw std::runtime_error("OpenGL vertex shader exception");
    }

    glCompileShader(fShaderAddress);

    GLint fSuccess = 0;
    glGetShaderiv(fShaderAddress, GL_COMPILE_STATUS, &fSuccess);

    if (fSuccess == GL_FALSE) {
        GLint logSize = 0;
        glGetShaderiv(fShaderAddress, GL_INFO_LOG_LENGTH, &logSize);
        
        GLchar *log;
        glGetShaderInfoLog(fShaderAddress, logSize, NULL, log);

        std::cout << "SHADER ERROR: ";
        for (size_t c=0; c < logSize; ++c) {
            std::cout << log[c];
        }
        std::cout << std::endl;

        throw std::runtime_error("OpenGL fragment shader exception");
    }

    this->programAddress = glCreateProgram();

    glAttachShader(this->programAddress, vShaderAddress);
    glAttachShader(this->programAddress, fShaderAddress);

    glLinkProgram(this->programAddress);

    GLint pSuccess = 0;
    glGetProgramiv(this->programAddress, GL_LINK_STATUS, &pSuccess);

    if (pSuccess == GL_FALSE) {
        GLint logSize = 0;
        glGetProgramiv(this->programAddress, GL_INFO_LOG_LENGTH, &logSize);

        GLchar *log;
        glGetShaderInfoLog(this->programAddress, logSize, NULL, log);

        std::cout << "SHADER LINK ERROR: ";
        for (size_t c=0; c < logSize; ++c) {
            std::cout << log[c];
        }
        std::cout << std::endl;

        throw std::runtime_error("OpenGL Shader program failed to link");
    }

    glDeleteShader(vShaderAddress);
    glDeleteShader(fShaderAddress);

    glUseProgram(this->programAddress);

    this->enabledColorUniformLocation     = glGetUniformLocation(this->programAddress, "enabledColor");
    this->modelMatrixUniformLocation      = glGetUniformLocation(this->programAddress, "model");
    this->projectionMatrixUniformLocation = glGetUniformLocation(this->programAddress, "projection");

    glUniform4fv(this->enabledColorUniformLocation, 1, glm::value_ptr(this->pixelColor)); // Sending opaque white to the shader
    
    glBindVertexArray(0); // Unbinding VAO first
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Initializing groups
    this->ram.fill(0);
    this->variableRegisters.fill(0);
    
    for (std::array<bool, 32> *col = this->displayState.begin(); col != this->displayState.end(); ++col) {
        col->fill(0);
    }

    // Inserting a built-in font in the ram
    this->ram[0x050] = 0xF0;
    this->ram[0x051] = 0x90;
    this->ram[0x052] = 0x90;
    this->ram[0x053] = 0x90;
    this->ram[0x054] = 0xF0, // 0

    this->ram[0x055] = 0x20;
    this->ram[0x056] = 0x60;
    this->ram[0x057] = 0x20;
    this->ram[0x058] = 0x20;
    this->ram[0x059] = 0x70; // 1

    this->ram[0x05A] = 0xF0;
    this->ram[0x05B] = 0x10;
    this->ram[0x05C] = 0xF0;
    this->ram[0x05D] = 0x80;
    this->ram[0x05E] = 0xF0; // 2

    this->ram[0x05F] = 0xF0;
    this->ram[0x060] = 0x10;
    this->ram[0x061] = 0xF0;
    this->ram[0x062] = 0x10;
    this->ram[0x063] = 0xF0; // 3

    this->ram[0x064] = 0x90;
    this->ram[0x065] = 0x90;
    this->ram[0x066] = 0xF0;
    this->ram[0x067] = 0x10;
    this->ram[0x068] = 0x10; // 4

    this->ram[0x069] = 0xF0;
    this->ram[0x06A] = 0x80;
    this->ram[0x06B] = 0xF0;
    this->ram[0x06C] = 0x10;
    this->ram[0x06D] = 0xF0;// 5

    this->ram[0x06E] = 0xF0;
    this->ram[0x06F] = 0x80;
    this->ram[0x070] = 0xF0;
    this->ram[0x071] = 0x90;
    this->ram[0x072] = 0xF0; // 6

    this->ram[0x073] = 0xF0;
    this->ram[0x074] = 0x10;
    this->ram[0x075] = 0x20;
    this->ram[0x076] = 0x40;
    this->ram[0x077] = 0x40; // 7

    this->ram[0x078] = 0xF0;
    this->ram[0x079] = 0x90;
    this->ram[0x07A] = 0xF0;
    this->ram[0x07B] = 0x90;
    this->ram[0x07C] = 0xF0; // 8

    this->ram[0x07D] = 0xF0;
    this->ram[0x07E] = 0x90;
    this->ram[0x07F] = 0xF0;
    this->ram[0x080] = 0x10;
    this->ram[0x081] = 0xF0, // 9

    this->ram[0x082] = 0xF0;
    this->ram[0x083] = 0x90;
    this->ram[0x084] = 0xF0;
    this->ram[0x085] = 0x90;
    this->ram[0x086] = 0x90, // A

    this->ram[0x087] = 0xE0;
    this->ram[0x088] = 0x90;
    this->ram[0x089] = 0xE0;
    this->ram[0x08A] = 0x90;
    this->ram[0x08B] = 0xE0; // B

    this->ram[0x08C] = 0xF0;
    this->ram[0x08D] = 0x80;
    this->ram[0x08E] = 0x80;
    this->ram[0x08F] = 0x80;
    this->ram[0x090] = 0xF0; // C

    this->ram[0x091] = 0xE0;
    this->ram[0x092] = 0x90;
    this->ram[0x093] = 0x90;
    this->ram[0x094] = 0x90;
    this->ram[0x095] = 0xE0; // D

    this->ram[0x096] = 0xF0;
    this->ram[0x097] = 0x80;
    this->ram[0x098] = 0xF0;
    this->ram[0x099] = 0x80;
    this->ram[0x09A] = 0xF0; // E
    
    this->ram[0x09B] = 0xF0;
    this->ram[0x09C] = 0x80;
    this->ram[0x09D] = 0xF0;
    this->ram[0x09E] = 0x80;
    this->ram[0x09F] = 0x80;  // F
}

void Chip8::load_program(const std::string &fileName) {
    std::ifstream programFile(fileName, std::ios::binary|std::ios::ate);
    if (!programFile.is_open()) {
        throw std::runtime_error("Program file not found : `" + fileName + "`");
    }

    std::streampos programSize = programFile.tellg();
    
    if (programSize > 4096-512) {
        throw std::runtime_error("Program is too big to fit in the memory (" + std::to_string(programSize) + " bytes)");
    }
    
    char *program = new char[programSize];

    programFile.seekg(0, std::ios::beg);
    programFile.read(program, programSize);
    programFile.close();

    for (uint16_t programByteId = 0; programByteId < programSize; ++programByteId) {
        this->ram[512 + programByteId] = static_cast<uint8_t>(program[programByteId]);
    }

    this->pc = 512;
}

void Chip8::run() {
    double maxChipFrequency = 0;

    double elapsedChipTime = 0;
    while (!glfwWindowShouldClose(this->display)) {
        elapsedChipTime -= glfwGetTime();

        glfwSwapBuffers(this->display);
        glfwPollEvents();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glm::mat4 projectionMatrix = glm::ortho(0.0f, 64.0f, 32.0f, 0.0f, -1.0f, 1.0f);

        glBindVertexArray(this->vaoAddress);
        glUseProgram(this->programAddress);

        glUniformMatrix4fv(this->projectionMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        // Updating render
        for (int i=0; i < 64; ++i) {
            for (int j=0; j < 32; ++j) {
                if (this->displayState[i][j]) {
                    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((float)i, (float)j, 0.0f));
                    glUniformMatrix4fv(this->modelMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }

        glBindVertexArray(0);
        glUseProgram(0);

        if (elapsedChipTime > 0) {
            continue; // Skip chip tick if running too fast. Can most likely be improved.
        }

        elapsedChipTime = maxChipFrequency;

        this->fetch();
        this->decode();
        this->execute();

        std::cout << std::flush;
    }
}

std::array<std::array<bool, 32>, 64> Chip8::get_display_state() {
    return this->displayState;
}

GLint Chip8::get_model_matrix_uniform_location() const {
    return this->modelMatrixUniformLocation;
}

GLint Chip8::get_projection_matrix_uniform_location() const {
    return this->projectionMatrixUniformLocation;
}

GLint Chip8::get_enabled_color_uniform_location() const {
    return this->enabledColorUniformLocation;
}

GLint Chip8::get_program_address() const {
    return this->programAddress;
}

char Chip8::read(const uint16_t &address) {
    return this->ram[address];
}

void Chip8::write(const uint16_t &address, const uint8_t &value) {
    this->ram[address] = value;
}

void Chip8::fetch() {
    this->rawInstruction = 0; // Reset next raw instruction

    // Causes segfault.
    this->rawInstruction |= static_cast<uint16_t>(this->ram[this->pc]) << 8;
    this->rawInstruction |= this->ram[this->pc+1]; // Offsetting program counter to next byte

    std::cout << "TRACK: Fetched raw instruction 0x" << get_bin_representation(this->rawInstruction) << "\n";
}

void Chip8::decode() {
    // Seems to be the culprit for above's segfault. Need to look at it tomorrow
    this->opcode         = (this->rawInstruction & 0b1111000000000000) >> 12; // First nibble is the opcode
    this->firstRegister  = (this->rawInstruction & 0b0000111100000000) >> 8; // Second nibble is a register's index
    this->secondRegister = (this->rawInstruction & 0b0000000011110000) >> 4; // Third nibble is also a register's index
    this->spriteSize     = this->rawInstruction & 0b0000000000001111; // Fourth nibble is used to specify sprite sizes

    this->immediateValue = static_cast<uint8_t>(this->rawInstruction & 0b0000000011111111); // Second byte is an immediate value

    this->immediateAddress = this->rawInstruction & 0b0000111111111111; // Second, third and fourth nibble are an immediate address
}

void Chip8::execute() {
    std::cout << "TRACK: opcode : " << get_hex_representation(this->opcode) << "\n";
    switch(this->opcode) {
        case 0x0:
            switch(this->rawInstruction) { // Whole instruction have fixed shape for most `0___` instructions
                case 0x00e0:
                    this->clear_screen();
                    break;
                
                case 0x00ee:
                    this->exit_subroutine();
                    break;
                
                default:
                    this->execute_machine_routine();
                    break;
            }
            break;
        
        case 0x1:
            this->jump();
            break;

        case 0x2:
            this->call_subroutine();
            break;
        
        case 0x3:
            this->skip_if_value();
            break;
        
        case 0x4:
            this->skip_if_not_value();
            break;
        
        case 0x5:
            this->skip_if_equals_register();
            break;
        
        case 0x6:
            this->set_var_register();
            break;

        case 0x7:
            this->add_var_register();
            break;

        case 0x8:
            switch(this->spriteSize) { // Fourth nibble differentiates `8__X` instructions
                case 0x0:
                    this->set_from_other_register();
                    break;
                
                case 0x1:
                    this->bin_or();
                    break;
                
                case 0x2:
                    this->bin_and();
                    break;
                
                case 0x3:
                    this->bin_xor();
                    break;

                case 0x4:
                    this->add_from_other_register();
                    break;
                
                case 0x5:
                    this->substract();
                    break;
                
                case 0x6:
                    this->bin_shift_right();
                    break;
                
                case 0x7:
                    this->substract_reverse();
                    break;

                case 0xE:
                    this->bin_shift_left();
                    break;
                
                default:
                    throw std::runtime_error("Unimplemented opcode starting by `8` : `" + std::to_string(this->opcode) + "`");
                    break;
            }
            break;
        
        case 0x9:
            this->skip_if_not_equals_register();
            break;

        case 0xA:
            this->set_index_register();
            break;
        
        case 0xB:
            this->jump_with_offset();
            break;
        
        case 0xC:
            this->random();
            break;

        case 0xD:
            this->draw();
            break;
        
        case 0xE:
            switch(this->immediateValue) { // Last 8 bits differentiate `E_XX` opcodes
                case 0x9E:
                    this->skip_if_key();
                    break;
                
                case 0xA1:
                    this->skip_if_not_key();
                    break;
                
                default:
                    throw std::runtime_error("Unimplemented opcode starting by `E` : `" + std::to_string(this->opcode) + "`");
                    break;
            }
            break;
        
        case 0xF:
            switch(this->immediateValue) { // Last 8 bits differenciate `F_XX` opcodes
                case 0x07:
                    this->set_reg_to_delay_timer();
                    break;

                case 0x0A:
                    this->get_key();
                    break;

                case 0x15:
                    this->set_delay_timer_to_reg();
                    break;
                
                case 0x18:
                    this->set_sound_timer_to_reg();
                    break;

                case 0x1E:
                    this->add_to_index_register();
                    break;
                
                case 0x29:
                    this->set_index_reg_to_character();
                    break;
                
                case 0x33:
                    this->decimal_conversion();
                    break;

                case 0x55:
                    this->memory_store();
                    break;
                
                case 0x65:
                    this->memory_load();
                    break;
                
                default:
                    throw std::runtime_error("Unimplemented opcode starting by `F` : `" + std::to_string(this->opcode) + "`");
            }
            break;

        default:
            throw std::runtime_error("Unimplemented opcode : `" + std::to_string(this->opcode) + "`");
            break;
    }
}

void Chip8::execute_machine_routine() {
    std::cout << "SKIPPING MACHINE ROUTINE EXECUTION\n";
}

void Chip8::clear_screen() {
    for (std::array<bool, 32> *col = this->displayState.begin(); col != this->displayState.end(); ++col) {
        col->fill(0);
    }

    this->pc += 2;

    std::cout << "TRACK: Cleared screen\n";
}

void Chip8::jump() {
    this->pc = this->immediateAddress;

    std::cout << "TRACK: jumped to " << (int)this->immediateAddress << "\n";
}

void Chip8::call_subroutine() {
    this->addressStack.push(this->pc);
    std::cout << "TRACK: Called a subroutine (pushed `" << this->pc << "` to the stack.)\n";
    this->pc = this->immediateAddress;
}

void Chip8::exit_subroutine() {
    std::cout << std::flush;
    this->pc = this->addressStack.top();
    this->addressStack.pop();
    std::cout << "TRACK: Exited a subroutine (Popped `" << this->pc << "` from the stack.)\n";

    this->pc += 2;
}

void Chip8::skip_if_value() {
    if (this->variableRegisters[this->firstRegister] == this->immediateValue) {
        this->pc += 4;
        std::cout << "TRACK: Skipped to `" << this->pc << "` because register " << (int)this->firstRegister << " is equal to immediate value `" << (int)this->immediateValue << "`\n";
    } else {
        this->pc += 2;
        std::cout << "TRACK: Didn't skip because register " << (int)this->firstRegister << " is different from value `" << (int)this->immediateValue << "`\n";
    }
}

void Chip8::skip_if_not_value() {
    if (this->variableRegisters[this->firstRegister] != this->immediateValue) {
        this->pc += 4;
        std::cout << "TRACK: Skipped to `" << this->pc << "` because register " << (int)this->firstRegister << " is different from immediate value `" << (int)this->immediateValue << "`\n";
    } else {
        this->pc += 2;
        std::cout << "TRACK: Didn't skip because register " << (int)this->firstRegister << " is equal to value `" << (int)this->immediateValue << "`\n";
    }
}

void Chip8::skip_if_equals_register() {
    if (this->variableRegisters[this->firstRegister] == this->variableRegisters[this->secondRegister]) {
        this->pc += 4;
        std::cout << "TRACK: Skipped to `" << this->pc << "` because register " << (int)this->firstRegister << " is equal to register " << (int)this->secondRegister << "\n";
    } else {
        this->pc += 2;
        std::cout << "TRACK: Didn't skip because register " << (int)this->firstRegister << "is different from register " << (int)this->secondRegister << "\n";
    }
}

void Chip8::skip_if_not_equals_register() {
    if (this->variableRegisters[this->firstRegister] != this->variableRegisters[this->secondRegister]) {
        this->pc += 4;
        std::cout << "TRACK: Skipped to `" << this->pc << "` because register " << (int)this->firstRegister << " is different from register " << (int)this->secondRegister << "\n";
    } else {
        this->pc += 2;
        std::cout << "TRACK: Didn't skip because register " << (int)this->firstRegister << "is equal to register " << (int)this->secondRegister << "\n";
    }
}

void Chip8::set_var_register() {
    this->variableRegisters[this->firstRegister] = this->immediateValue;

    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th var register to " << (int)this->immediateValue << "\n";
}

void Chip8::add_var_register() {
    this->variableRegisters[this->firstRegister] += this->immediateValue;

    this->pc += 2;

    std::cout << "TRACK: Added " << (int)this->immediateValue << " to " << (int)this->firstRegister << "th var register\n";
}

void Chip8::set_from_other_register() {
    this->variableRegisters[this->firstRegister] = this->variableRegisters[this->secondRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to " << (int)this->secondRegister << "th's value (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::bin_or() {
    this->variableRegisters[this->firstRegister] |= this->variableRegisters[this->secondRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to it's value |OR|" << (int)this->secondRegister << "th's one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::bin_and() {
    this->variableRegisters[this->firstRegister] &= this->variableRegisters[this->secondRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to it's value &AND&" << (int)this->secondRegister << "th's one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::bin_xor() {
    this->variableRegisters[this->firstRegister] ^= this->variableRegisters[this->secondRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to it's value ^XOR^ " << (int)this->secondRegister << "th's one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::add_from_other_register() {
    uint16_t additionResult = this->variableRegisters[this->firstRegister] + this->variableRegisters[this->secondRegister];
    if (additionResult > 255) {
        this->variableRegisters[0xf] = 1;
        additionResult %= 256;
    } else {
        this->variableRegisters[0xf] = 0;
    }

    this->variableRegisters[this->firstRegister] = static_cast<uint8_t>(additionResult);
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to it's value +PLUS+ " << (int)this->secondRegister << "th's one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::substract() {
    if (this->variableRegisters[this->firstRegister] > this->variableRegisters[this->secondRegister]) {
        this->variableRegisters[0xf] = 1;
    } else {
        this->variableRegisters[0xf] = 0;
    }
    
    this->variableRegisters[this->firstRegister] -= this->variableRegisters[this->secondRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to it's value -MINUS- " << (int)this->secondRegister << "th's one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::substract_reverse() {
    if (this->variableRegisters[this->secondRegister] > this->variableRegisters[this->firstRegister]) {
        this->variableRegisters[0xf] = 1;
    } else {
        this->variableRegisters[0xf] = 0;
    }

    this->variableRegisters[this->firstRegister] = this->variableRegisters[this->secondRegister] - this->variableRegisters[this->firstRegister];
    this->pc += 2;

    std::cout << "TRACK: Set " << (int)this->firstRegister << "th register to " << (int)this->secondRegister << "th register's value -MINUS- it's own one (`" << (int)this->variableRegisters[this->firstRegister] << "`)\n";
}

void Chip8::bin_shift_left() {
    this->variableRegisters[0xf] = (this->secondRegister & 0b10000000) >> 7;
    this->variableRegisters[this->firstRegister] = (this->variableRegisters[this->secondRegister]) << 1;
    this->pc += 2;

    std::cout << "TRACK: Left-shifted " << (int)this->firstRegister << "th register. Saved result (`" << this->variableRegisters[this->firstRegister] << "`) to " << this->secondRegister << "th register.";
}

void Chip8::bin_shift_right() {
    this->variableRegisters[0xf] = this->secondRegister & 1;
    this->variableRegisters[this->firstRegister] = (this->variableRegisters[this->secondRegister]) >> 1;
    this->pc += 2;

    std::cout << "TRACK: Right-shifted " << (int)this->firstRegister << "th register. Saved result (`" << this->variableRegisters[this->firstRegister] << "`) to " << this->secondRegister << "th register.";
}

void Chip8::set_index_register() {
    this->indexRegister = this->immediateAddress;

    this->pc += 2;
    
    std::cout << "TRACK: Set index register to " << (int)this->immediateAddress << "\n";
}

void Chip8::jump_with_offset() {
    this->pc = immediateAddress + this->variableRegisters[0];

    std::cout << "TRACK: Jumped to `" << this->immediateAddress << " + " << this->variableRegisters[0] << "` (`" << this->pc << "`\n)";
}

void Chip8::random() {
    this->variableRegisters[this->firstRegister] = static_cast<uint8_t>(this->randomDistribution(this->randomEngine)) & this->immediateValue;
    this->pc += 2;

    std::cout << "TRACK: Put random value `" << (int)this->variableRegisters[this->firstRegister] << "` in " << (int)this->firstRegister << "th register\n";
}

void Chip8::draw() {
    uint8_t xCoord = this->variableRegisters[this->firstRegister] %64;
    uint8_t yCoord = this->variableRegisters[this->secondRegister]%32;

    this->variableRegisters[0xf] = 0;

    for (uint16_t rowId = 0; rowId < this->spriteSize; ++rowId) {
        if (yCoord+rowId >= 32) { // Sprite can't vertically wrap
            break;
        }

        uint8_t rowData = this->ram[this->indexRegister+rowId];

        /*
         * 0 - 0b00000001
         * 1 - 0b00000010
         * 2 - 0b00000100
         * 3 - 0b00001000
         * 4 - 0b00010000
         * 5 - 0b00100000
         * 6 - 0b01000000
         * 7 - 0b10000000
         */
        for (uint8_t pixelRank = 7; pixelRank < 255; pixelRank--) {
            uint8_t xOffset = 7-pixelRank;
            std::cout << "DEBUG: pixel rank : " << (int)pixelRank << std::endl;
            if (xCoord + xOffset >= 64) { // Sprite can't horizontally wrap
                continue;
            }

            uint8_t selector = 1 << pixelRank;

            bool pixelValue = (rowData & selector) >> pixelRank;

            if (pixelValue == 1) {
                if (this->displayState[xCoord+xOffset][yCoord+rowId]) {
                    this->variableRegisters[0xf] = 1;
                }

                this->displayState[xCoord+xOffset][yCoord+rowId] = !this->displayState[xCoord+xOffset][yCoord+rowId];
            }
        }
    }

    this->pc  += 2;

    std::cout << "TRACK: Drew " << (int)this->spriteSize << "-tall sprite @ (" << (int)xCoord << ", " << (int)yCoord << ")\n";
}

void Chip8::skip_if_key() {
    if (this->variableRegisters[this->firstRegister] == 0x1) {
        if (glfwGetKey(this->display, GLFW_KEY_1) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `1` (`1` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `1` (`1` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x2) {
        if (glfwGetKey(this->display, GLFW_KEY_2) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `2` (`2` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `2` (`2` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x3) {
        if (glfwGetKey(this->display, GLFW_KEY_3) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `3` (`3` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `3` (`3` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xC) {
        if (glfwGetKey(this->display, GLFW_KEY_4) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `C` (`4` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `C` (`4` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x4) {
        if (glfwGetKey(this->display, GLFW_KEY_Q) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `4` (`Q` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `4` (`Q` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x5) {
        if (glfwGetKey(this->display, GLFW_KEY_W) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `5` (`W` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `5` (`W` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x6) {
        if (glfwGetKey(this->display, GLFW_KEY_E) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `6` (`E` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `6` (`E` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xD) {
        if (glfwGetKey(this->display, GLFW_KEY_R) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `D` (`R` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `D` (`R` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x7) {
        if (glfwGetKey(this->display, GLFW_KEY_A) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `7` (`A` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `7` (`A` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x8) {
        if (glfwGetKey(this->display, GLFW_KEY_S) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `8` (`S` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `8` (`S` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x9) {
        if (glfwGetKey(this->display, GLFW_KEY_D) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `9` (`D` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `9` (`D` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xE) {
        if (glfwGetKey(this->display, GLFW_KEY_F) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `E` (`F` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `E` (`F` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xA) {
        if (glfwGetKey(this->display, GLFW_KEY_Z) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `A` (`Z` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `A` (`Z` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x0) {
        if (glfwGetKey(this->display, GLFW_KEY_X) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `0` (`X` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `0` (`X` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xB) {
        if (glfwGetKey(this->display, GLFW_KEY_S) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `B` (`C` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `B` (`C` on keyboard) wasn't PRESS.";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xF) {
        if (glfwGetKey(this->display, GLFW_KEY_V) == GLFW_PRESS) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `F` (`V` on keyboard) was PRESS.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `F` (`V` on keyboard) wasn't PRESS.";
        }
    }
    
    this->pc += 2;
}

void Chip8::skip_if_not_key() {
    if (this->variableRegisters[this->firstRegister] == 0x1) {
        if (glfwGetKey(this->display, GLFW_KEY_1) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `1` (`1` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `1` (`1` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x2) {
        if (glfwGetKey(this->display, GLFW_KEY_2) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `2` (`2` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `2` (`2` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x3) {
        if (glfwGetKey(this->display, GLFW_KEY_3) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `3` (`3` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `3` (`3` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xC) {
        if (glfwGetKey(this->display, GLFW_KEY_4) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `C` (`4` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `C` (`4` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x4) {
        if (glfwGetKey(this->display, GLFW_KEY_Q) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `4` (`Q` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `4` (`Q` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x5) {
        if (glfwGetKey(this->display, GLFW_KEY_W) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `5` (`W` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `5` (`W` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x6) {
        if (glfwGetKey(this->display, GLFW_KEY_E) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `6` (`E` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `6` (`E` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xD) {
        if (glfwGetKey(this->display, GLFW_KEY_R) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `D` (`R` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `D` (`R` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x7) {
        if (glfwGetKey(this->display, GLFW_KEY_A) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `7` (`A` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `7` (`A` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x8) {
        if (glfwGetKey(this->display, GLFW_KEY_S) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `8` (`S` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `8` (`S` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x9) {
        if (glfwGetKey(this->display, GLFW_KEY_D) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `9` (`D` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `9` (`D` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xE) {
        if (glfwGetKey(this->display, GLFW_KEY_F) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `E` (`F` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `E` (`F` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xA) {
        if (glfwGetKey(this->display, GLFW_KEY_Z) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `A` (`Z` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `A` (`Z` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0x0) {
        if (glfwGetKey(this->display, GLFW_KEY_X) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `0` (`X` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `0` (`X` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xB) {
        if (glfwGetKey(this->display, GLFW_KEY_C) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `B` (`C` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `B` (`C` on keyboard) wasn't RELEASE.\n";
        }
    } else if (this->variableRegisters[this->firstRegister] == 0xF) {
        if (glfwGetKey(this->display, GLFW_KEY_V) == GLFW_RELEASE) {
            this->pc += 2;
            std::cout << "TRACK: Skipped because key `F` (`V` on keyboard) was RELEASE.\n";
        } else {
            std::cout << "TRACK: Didn't skip because key `F` (`V` on keyboard) wasn't RELEASE.\n";
        }
    }
    
    this->pc += 2;
}

void Chip8::set_reg_to_delay_timer() {
    this->variableRegisters[this->firstRegister] = this->delayTimer;
    this->pc += 2;

    std::cout << "TRACK: Set " << this->firstRegister << "th register to the value of the delay timer (`" << this->delayTimer << "`)\n";
}

void Chip8::set_delay_timer_to_reg() {
    this->delayTimer = this->variableRegisters[this->firstRegister];
    this->pc += 2;

    std::cout << "TRACK: Set delay timer to " << this->firstRegister << "th register's value (`" << this->delayTimer << "`)\n";
}

void Chip8::set_sound_timer_to_reg() {
    this->soundTimer = this->variableRegisters[this->firstRegister];
    this->pc += 2;

    std::cout << "TRACK: Set sound timer to " << this->firstRegister << "th register's value (`" << this->soundTimer << "`)\n";
}

void Chip8::add_to_index_register() {
    this->indexRegister += this->variableRegisters[this->firstRegister];
    this->pc += 2;

    std::cout << "TRACK: Set index register to " << this->firstRegister << "th register's value (`" << this->indexRegister << "`)\n";
}

void Chip8::get_key() {
    if (glfwGetKey(this->display, GLFW_KEY_1) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x1;
        std::cout << "TRACK: Exiting getkey because key `1` (`1` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_2) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x2;
        std::cout << "TRACK: Exiting getkey because key `2` (`2` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_3) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x3;
        std::cout << "TRACK: Exiting getkey because key `3` (`3` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_4) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xC;
        std::cout << "TRACK: Exiting getkey because key `C` (`4` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_Q) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x4;
        std::cout << "TRACK: Exiting getkey because key `4` (`Q` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_W) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x5;
        std::cout << "TRACK: Exiting getkey because key `5` (`W` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_E) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x6;
        std::cout << "TRACK: Exiting getkey because key `6` (`E` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_R) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xD;
        std::cout << "TRACK: Exiting getkey because key `D` (`R` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_A) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x7;
        std::cout << "TRACK: Exiting getkey because key `7` (`A` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_S) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x8;
        std::cout << "TRACK: Exiting getkey because key `8` (`S` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_D) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x9;
        std::cout << "TRACK: Exiting getkey because key `9` (`D` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_F) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xE;
        std::cout << "TRACK: Exiting getkey because key `E` (`F` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_Z) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xA;
        std::cout << "TRACK: Exiting getkey because key `A` (`Z` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_X) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0x0;
        std::cout << "TRACK: Exiting getkey because key `0` (`X` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_C) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xB;
        std::cout << "TRACK: Exiting getkey because key `B` (`C` on keyboard)\n";
    } else if (glfwGetKey(this->display, GLFW_KEY_V) == GLFW_PRESS) {
        this->pc += 2;
        this->variableRegisters[this->firstRegister] = 0xF;
        std::cout << "TRACK: Exiting getkey because key `F` (`V` on keyboard)\n";
    }

    std::cout << "TRACK: Getkey didn't detect any key\n";
}

void Chip8::set_index_reg_to_character() {
    this->indexRegister = 0x50 + 5*this->variableRegisters[this->firstRegister];
    this->pc += 2;

    std::cout << "Set index register to the position of system font's " << get_hex_representation(this->variableRegisters[this->firstRegister]) << " character\n";
}

void Chip8::decimal_conversion() {
    uint8_t numberToConvert = this->variableRegisters[this->firstRegister];
    this->ram[this->indexRegister+2]   = (numberToConvert    ) % 10;
    this->ram[this->indexRegister+1] = (numberToConvert/10 ) % 10;
    this->ram[this->indexRegister] =  numberToConvert/100;
    this->pc += 2;

    std::cout << "TRACK: Filled ram from " << this->indexRegister << " to " << this->indexRegister+2 << " with decimal digits of `" << (int)this->variableRegisters[this->firstRegister] << "`\n";
}

void Chip8::memory_store() {
    for (uint8_t i=0; i <= this->firstRegister; ++i) {
        this->ram[this->indexRegister + i] = this->variableRegisters[i];
    }

    this->pc += 2;
    std::cout << "TRACK: Saved memory from " << this->indexRegister << " to " << this->indexRegister + this->firstRegister << " on the ram (" << (int)this->firstRegister << " registers saved)\n";
}

void Chip8::memory_load() {
    for (uint8_t i=0; i <= this->firstRegister; ++i) {
        this->variableRegisters[i] = this->ram[this->indexRegister+i];
    }

    this->pc += 2;
    std::cout << "TRACK: Loaded memory from " << this->indexRegister << " to " << this->indexRegister + this->firstRegister << " (" << (int)this->firstRegister << " registers)\n";
}

void Chip8::glfw_error_callback(int error, const char *description) {
    std::cerr << "GLFW Error : " << description << std::endl;
}

void Chip8::glfw_frame_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Chip8::glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    Chip8 *emu = static_cast<Chip8 *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        std::cout << "PRESSED L" << "\n";
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        std::string pixels = "PIXELS :\n";
        for (size_t j=0; j < 32; ++j) {
            for (size_t i=0; i < 64; ++i) {
                if (emu->get_display_state()[i][j]) {
                    pixels.append("█");
                } else {
                    pixels.append(" ");
                }
            }
            pixels.append("\n");
        }
        std::cout << pixels << "\nUNIFORMS :\n";
        std::cout << "Model matrix     : " << emu->get_model_matrix_uniform_location()      << "\n";
        std::cout << "Enabled color    : " << emu->get_enabled_color_uniform_location()     << "\n";
        std::cout << "Projection matrix: " << emu->get_projection_matrix_uniform_location() << "\n";
        
        std::cout << "\nOTHER OPENGL FIELDS :\n";
        std::cout << "Program Address: " << emu->get_program_address() << std::endl;
    }
}
