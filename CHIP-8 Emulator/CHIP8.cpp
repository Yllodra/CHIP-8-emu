#include "CHIP8.h"

#include <assert.h>
#include <fstream>
#include <iostream>
#include <random>


CHIP8::CHIP8(void): m_distribution(0, 0xFF),
                    m_opcode(0),
                    m_I(0),
                    m_pc(0x200), // Program/Game ROM starts at address 0x200.
                    m_delay_timer(0),
                    m_sound_timer(0),
                    m_sp(0),
                    m_draw_flag(true) // Initial draw for clearing purposes
{
    // Note that many of the "initializations" can also be done implicitly but due to lack of
    // testing doing it explicitly is favored.
    m_V.fill(0);
    m_gfx.fill(false);
    m_stack.fill(0);
    m_key.fill(false);
    m_memory.fill(0);

    // Prepare to enter the fontset into memory.
    std::array<unsigned char, 80> fontset =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0, looks like 0 in binary form.
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

    // Enter the fontset into memory.
    for (int i = 0; i < 80; ++i)
        m_memory.at(i) = fontset.at(i);

    // Start randomization engine.
    std::random_device rd;
    m_mersenne_twister.seed(rd());
}


CHIP8::~CHIP8(void)
{
}

void CHIP8::loadGame(std::string file_name)
{
    std::ifstream file(file_name, std::ifstream::binary);
    int i = 0x200;
    while (file.good())
    {
        m_memory.at(i) = file.get();
        ++i;
    }
    file.close();
}

void CHIP8::emulateCycle()
{
    // Read next opcode.
    m_opcode = m_memory.at(m_pc) << 8 | m_memory.at(m_pc+1);
    m_pc += 2;

    // Decode opcode.
    bool success = decodeOpcode();

    if (!success)
        return;

    // Update timers.
    if (m_delay_timer > 0)
        --m_delay_timer;
    if (m_sound_timer > 0)
    {
        if (m_sound_timer == 1)
            std::cout << "BEEP!" << "\7" << std::endl;
        --m_sound_timer;
    }
}

void CHIP8::setKeys(unsigned short key, bool state)
{
    assert(key >= 0 && key <= 15);
    m_key.at(key) = state;
}

std::array<bool, 2048> CHIP8::gfx() const
{
    return m_gfx;
}

bool CHIP8::draw_flag() const
{
    return m_draw_flag;
}

void CHIP8::draw_flag(bool draw_flag)
{
    m_draw_flag = draw_flag;
}

bool CHIP8::decodeOpcode()
{
    switch (m_opcode & 0xF000) // Bitwise AND with 0xF000 means we only read the first hexadecimal value.
    {
        case 0x0000:
        {
            switch (m_opcode) // implicit &0x0FFF, we only care about the last three values.
            {
                // Clears the screen.
                case 0x00E0: // CLS
                {
                    m_gfx.fill(0);
                    m_draw_flag = true;
                    break;
                }
                // Returns from a subroutine.
                case 0x00EE: // RET
                {
                    m_pc = m_stack.at(m_sp);
                    --m_sp;
                    break;
                }
                // case 0x0NNN:
                // Calls RCA 1802 program at address NNN.
                default:
                {
                    std::cout << "Opcode 0x0NNN not implemented." << std::endl;
                    break;
                }
            }
            break;
        }
        // case 0x1NNN
        // Jumps to address NNN.
        case 0x1000: // JP addr
        {
            m_pc = m_opcode & 0x0FFF;
            break;
        }
        // case 0x2NNN
        // Calls subroutine at NNN.
        case 0x2000: // CALL addr
        {
            ++m_sp;
            m_stack.at(m_sp) = m_pc;
            m_pc = m_opcode & 0x0FFF;
            break;
        }
        // case 0x3XNN
        // Skips the next instruction if VX equals NN.
        case 0x3000: // SE Vx, byte
        {
            if (m_V.at((m_opcode & 0x0F00) >> 8) == (m_opcode & 0x00FF))
                m_pc+=2;
            break;
        }
        // case 0x4XNN
        // Skips the next instruction if VX doesn't equal NN.
        case 0x4000: // SNE Vx, byte
        {
            if (m_V.at((m_opcode & 0x0F00) >> 8) != (m_opcode & 0x00FF))
                m_pc+=2;
            break;
        }
        case 0x5000:
        {
            switch (m_opcode & 0x000F)
            {
                // case 0x5XY0
                // Skips the next instruction if VX equals VY.
                case 0x0000: // SE, Sx, Vy
                {
                    if (m_V.at((m_opcode & 0x0F00) >> 8) == m_V.at((m_opcode & 0x00F0) >> 4))
                        m_pc+=2;
                    break;
                }
                default:
                {
                    std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
                    break;
                }
            }
            break;
        }
        // case 0x6XNN
        // Sets VX to NN.
        case 0x6000: // LD Vx, byte
        {
            m_V.at((m_opcode & 0x0F00) >> 8) = m_opcode & 0x00FF;
            break;
        }
        // case 0x7XNN
        // Adds NN to VX.
        case 0x7000: // ADD Vx, byte
        {
            m_V.at((m_opcode & 0x0F00) >> 8) += m_opcode & 0x00FF;
            break;
        }
        case 0x8000:
        {
            switch (m_opcode & 0x000F)
            {
                // case 0x8XY0
                // Sets VX to the value of VY.
                case 0x0000: // LD Vx, Vy
                {
                    m_V.at((m_opcode & 0x0F00) >> 8) = m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY1
                // Sets VX to VX or VY.
                case 0x0001: // OR Vx, Vy
                {
                    m_V.at((m_opcode & 0x0F00) >> 8) |= m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY2
                // Sets VX to VX bitwise AND VY.
                case 0x0002: // AND Vx, Vy
                {
                    m_V.at((m_opcode & 0x0F00) >> 8) &= m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY3
                // Sets VX to VX xor VY.
                case 0x0003: // XOR Vx, Vy
                {
                    m_V.at((m_opcode & 0x0F00) >> 8) ^= m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY4
                // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
                case 0x0004: // ADD Vx, Vy
                {
                    if (m_V.at((m_opcode & 0x00F0) >> 4) > (0xFF - m_V.at((m_opcode & 0x0F00) >> 8)))
                        m_V.at(0xF) = 1; // There is a carry.
                    else
                        m_V.at(0xF) = 0; // There is no carry.
                    m_V.at((m_opcode & 0x0F00) >> 8) += m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY5
                // VY is subtracted from VX. VF is set to 0 when there's a borrow and 1 when there isn't.
                case 0x0005: // SUB Vx, Vy
                {
                    if (m_V.at((m_opcode & 0x0F00) >> 8) < m_V.at((m_opcode & 0x00F0) >> 4))
                        m_V.at(0xF) = 0; // There is a borrow.
                    else
                        m_V.at(0xF) = 1; // There is no borrow.
                    m_V.at((m_opcode & 0x0F00) >> 8) -= m_V.at((m_opcode & 0x00F0) >> 4);
                    break;
                }
                // case 0x8XY6
                // Shifts VX right by one. VF is set to the value of the least significant bit of VX before
                // the shift.
                case 0x0006: // SHR Vx {, Vy}
                {
                    m_V.at(0xF) = m_V.at((m_opcode & 0x0F00) >> 8) & 0x01;
                    m_V.at((m_opcode & 0x0F00) >> 8) >>= 1;
                    break;
                }
                // case 0x8XY7
                // Sets VX to VY minus VX. VF is set to 0 when there's a borrow and 1 when there isn't.
                case 0x0007: // SUBN Vx, Vy
                {
                    if (m_V.at((m_opcode & 0x0F00) >> 8) > m_V.at((m_opcode & 0x00F0) >> 4))
                        m_V.at(0xF) = 0; // There is a borrow.
                    else
                        m_V.at(0xF) = 1; // There is no borrow.
                    m_V.at((m_opcode & 0x0F00) >> 8) = m_V.at((m_opcode & 0x00F0) >> 4) - m_V.at((m_opcode & 0x0F00) >> 8);
                    break;
                }
                // case 0x8XYE
                // Shifts VX left by one. VF is set to the value of the most significant bit of VX before
                // the shift.
                case 0x000E: // SHL Vx {, Vy}
                {
                    m_V.at(0xF) = m_V.at((m_opcode & 0x0F00) >> 8) >> 7;
                    m_V.at((m_opcode & 0x0F00) >> 8) <<= 1;
                    break;
                }
                default:
                {
                    std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
                    break;
                }
            }
            break;
        }
        case 0x9000:
        {
            switch (m_opcode & 0x000F)
            {
                // case 0x9XY0
                // Skips the next instruction if VX doesn't equal VY.
                case 0x0000: // SNE Vx, Vy
                {
                    if (m_V.at((m_opcode & 0x0F00) >> 8) != m_V.at((m_opcode & 0x00F0) >> 4))
                        m_pc+=2;
                    break;
                }
                default:
                {
                    std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
                    break;
                }
            }
            break;
        }
        // case 0xANNN
        // Sets I to the address NNN.
        case 0xA000: // LD I, addr
        {
            m_I = m_opcode & 0x0FFF;
            break;
        }
        // case 0xBNNN
        // Jumps to the address NNN plus V0.
        case 0xB000: // JP V0, addr
        {
            m_pc = (m_opcode & 0x0FFF) + m_V.at(0x0);
            break;
        }
        // case 0xCXNN
        // Sets VX to a random number by generating a "random" number between 0x00 and 0xFF and then using bitwise AND with NN as mask.
        case 0xC000: // RND Vx, byte
        {
            int random_number = m_distribution(m_mersenne_twister);
            random_number &= (m_opcode & 0x00FF); // Bitwise AND with NN.
            m_V.at((m_opcode & 0x0F00) >> 8) = random_number;
            break;
        }
        // case 0xDXYN
        // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N
        // pixels. Each row of 8 pixels is read as bit-coded (with the most significant bit of each
        // byte displayed on the left) starting from memory location I; I value doesn't change
        // after the exectution of this instruction. VF is set to 1 if any screen pixels are
        // flipped from set to unset when the sprite is drawn, and to 0 if that doesn't happen.
        case 0xD000: // DRW Vx, Vy, nibble
        {
            unsigned short x_coordinate; // X-coordinate.
            unsigned short y_coordinate; // Y-coordinate.
            int rows; // Number of rows to draw.

            // Set coordinates and wrap as needed.
            x_coordinate = m_V.at((m_opcode & 0x0F00) >> 8) % 64;
            y_coordinate = m_V.at((m_opcode & 0x00F0) >> 4) % 32;
            rows = m_opcode & 0x000F;

            assert(x_coordinate < 64 && x_coordinate >= 0);
            assert(y_coordinate < 32 && y_coordinate >= 0);

            m_V.at(0xF) = 0; // Collision flag.

            for (int y = 0; y < rows; ++y) // Number of rows to draw.
            {
                unsigned char pixel_byte = m_memory.at(m_I+y);
                for (int x = 0; x < 8; ++x) // 8 pixels width.
                {
                    int coordinate = x_coordinate + x + ((y_coordinate + y) * 64);
                    if (coordinate >= 2048)
                        break;
                    if((pixel_byte & (0x80 >> x)) != 0)
					{
						if(m_gfx.at(coordinate) == true)
							m_V.at(0xF) = 1;                                    
						m_gfx.at(coordinate) ^= true;
					}
                }
            }
            
            m_draw_flag = true;
            break;
        }
        case 0xE000:
        {
            switch (m_opcode & 0x00FF)
            {
                // case 0xEX9E
                // Skips the next instruction if the key stored in VX is pressed.
                case 0x009E: // SKP, Vx
                {
                    if (m_key.at(m_V.at((m_opcode & 0x0F00) >> 8)))
                        m_pc += 2;
                    break;
                }
                // case 0xEXA1
                // Skips the next instruction if the key stored in VX isn't pressed.
                case 0x00A1: //SKNP Vx
                {
                    if (!m_key.at(m_V.at((m_opcode & 0x0F00) >> 8)))
                        m_pc += 2;
                    break;
                }
                default:
                {
                    std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
                    break;
                }
            }
            break;
        }
        case 0xF000:
        {
            switch (m_opcode & 0x00FF)
            {
                // case 0xFX07
                // Sets VX to the value of the delay timer.
                case 0x0007: // LD Vx, DT
                {
                    m_V.at((m_opcode & 0x0F00) >> 8) = m_delay_timer;
                    break;
                }
                // case 0xFX0A
                // A key press is awaited, and then stored in VX.
                case 0x000A: // LD Vx, K
                {
                    bool key_pressed = false;
                    for (int i = 0; i <= 0xF; ++i)
                    {
                        if (m_key.at(i))
                        {
                            m_V.at((m_opcode & 0x0F00) >> 8) = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed)
                    {
                        m_pc -= 2;
                        return false;
                    }
                    break;
                }
                // case 0xFX15
                // Sets the delay timer to VX.
                case 0x0015: // LD DT, Vx
                {
                    m_delay_timer = m_V.at((m_opcode & 0x0F00) >> 8);
                    break;
                }
                // case 0xFX18
                // Sets the sound timer to VX.
                case 0x0018: // LD ST, Vx
                {
                    m_sound_timer = m_V.at((m_opcode & 0x0F00) >> 8);
                    break;
                }
                // case 0xFX1E
                // Adds VX to I. If range is overflown (> 0xFFF) set VF to 1, else set to 0.
                case 0x001E: // ADD I, Vx
                {
                    // TODO: Verify how overflow is handled.
                    if (m_I + m_V.at((m_opcode & 0x0F00) >> 8) > 0xFFF)
                        m_V.at(0xF) = 1; // Overflow.
                    else
                        m_V.at(0xF) = 0; // No overflow.

                    m_I += m_V.at((m_opcode & 0x0F00) >> 8);
                    break;
                }
                // case 0xFX29
                // Sets I to the location of the sprite for the character in VX. Characters 0-F (in 
                // hexadecimal) are represented by a 4x5 font.
                case 0x0029: // LD F, Vx
                {
                    // Stored font starts at memory location 0*5 for 0x0, then the next character 0x1 is 5 steps away, ie 1*5, next is at 2*5, and so on.
                    m_I = m_V.at((m_opcode & 0x0F00) >> 8) * 5;
                    break;
                }
                // case 0xFX33
                // Stores the binary-coded decimal representation of VX, with the most significant of three
                // digits at the address in I, the middle digit at I plus 1, and the least significant digit at
                // I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit
                // in memory at location in I, the ten digit location I+1, and the ones digit at location I+2.)
                case 0x0033: // LD B, Vx
                {
                    m_memory.at(m_I) = (m_V.at((m_opcode & 0x0F00) >> 8) % 1000) / 100 ; // Hundreds.
                    m_memory.at(m_I+1) = (m_V.at((m_opcode & 0x0F00) >> 8) % 100) / 10; // Tens.
                    m_memory.at(m_I+2) = m_V.at((m_opcode & 0x0F00) >> 8) % 10; // Ones.
                    break;
                }
                // case 0xFX55
                // Stores V0 to VX in memory starting at address I. Set I to I+X+1.
                case 0x0055: // LD [I], Vx
                {
                    int end = (m_opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= end; ++i)
                        m_memory.at(m_I+i) = m_V.at(i);

                    m_I += end + 1;
                    break;
                }
                // case 0xFX65
                // Fills V0 to VX with values from memory starting at address I. Set I to I+X+1.
                case 0x0065: // LD Vx, [I]
                {
                    int end = (m_opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= end; ++i)
                        m_V.at(i) = m_memory.at(m_I+i);
                    m_I += end + 1;
                    break;
                }
                default:
                {
                    std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
                    break;
                }
            }
            break;
        }
        default:
        {
            std::cout << "Opcode " << m_opcode << " (decimal) not recognized." << std::endl;
            break;
        }
    }
    return true;
}