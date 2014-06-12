#pragma once
#include <array>
#include <random>

class CHIP8
{
public:
    CHIP8(void);
    ~CHIP8(void);
    void loadGame(std::string);
    void emulateCycle();

    
    /* Keys are 0x0 to 0xF represented as 0 to 15. State is 0 released, 1 pressed.
    Keypad    >>>   Index
    1 2 3 c   >>>   0x1 0x2 0x3 0xC
    4 5 6 d   >>>   0x4 0x5 0x6 0xD
    7 8 9 e   >>>   0x7 0x8 0x9 0xE
    a 0 b f   >>>   0xA 0x0 0xB 0xF
    */
    void setKeys(unsigned short key, bool state);

    std::array<bool, 2048> gfx() const;
    bool draw_flag() const; // Draw flag getter.
    void draw_flag(bool); // Draw flag setter.

private:
    bool decodeOpcode();

    unsigned short m_opcode; // Opcode

    /* Memory map
    0x000-0x1FF - CHIP-8 interpreter (contains font set in emu)
    0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
    0x200-0xFFF - Program ROM and work RAM
    */
    std::array<unsigned char, 4096> m_memory; // Memory
    
    // 8-bit general purpose CPU registers, V[0xF] is a carry flag.
    std::array<unsigned char, 16> m_V; // CPU registers.

    // Index register (used for modifying operand addresses).
    // The contents of an index register is added to or subtracted from an immediate address to
    // form the "effective" address of the actual data (operand).
    unsigned short m_I; // Index register (used for modifying operand addresses).

    unsigned short m_pc; // program counter, between 0x000 and 0xFFF

    std::array<bool, 2048> m_gfx; //Black and white screen, 2048 pixels in 64 x 32 resolution.

    // Timer registers. Counts down at 60 Hz. When set above 0 they will count down to 0.
    unsigned char m_delay_timer; // Used for timing of events.
    unsigned char m_sound_timer; // The system's buzzer sounds when it reaches 0.

    std::array<unsigned short, 16> m_stack; // Remembers memory locations on jumps or calls of a subroutine.
    unsigned short m_sp; // Stack pointer, remembers which level of the stack is used.

    /* HEX based keypad. Layout:
    1 2 3 c
    4 5 6 d
    7 8 9 e
    a 0 b f
    */
    std::array<bool, 16> m_key; // HEX based (0x0-0xF) keypad states.

    bool m_draw_flag; // Indicates whether drawing should be done.

    std::mt19937 m_mersenne_twister;
    std::uniform_int_distribution<int> m_distribution;
};
