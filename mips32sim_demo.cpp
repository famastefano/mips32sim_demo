#include "mips32/test/helpers/test_cpu_instructions.hpp"

#include <mips32/machine.hpp>
#include <mips32/literals.hpp>

#include <fmt/format.h>

#include <vector>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <memory>
#include <cstdint>
#include <iostream>

struct ConsoleIODevice final : virtual public mips32::IODevice
{
    FILE* log;
    ConsoleIODevice() noexcept;
    ~ConsoleIODevice() override;

    void print_integer(std::uint32_t value) noexcept override;
    void print_float(float value) noexcept override;
    void print_double(double value) noexcept override;
    void print_string(char const* string) noexcept override;
    void read_integer(std::uint32_t* value) noexcept override;
    void read_float(float* value) noexcept override;
    void read_double(double* value) noexcept override;
    void read_string(char* string, std::uint32_t max_count) noexcept override;
};

struct CSTDIOFileHandler final : virtual public mips32::FileHandler
{
    CSTDIOFileHandler() noexcept;
    ~CSTDIOFileHandler() override;

    std::uint32_t open(char const* name, char const* flags) noexcept override;
    std::uint32_t read(std::uint32_t fd, char * dst, std::uint32_t count) noexcept override;
    std::uint32_t write(std::uint32_t fd, char const* src, std::uint32_t count) noexcept override;
    void close(std::uint32_t fd) noexcept override;

    std::vector<FILE*> files;
};

struct MachineDataPlotter
{
    mips32::MachineInspector& inspector;
    std::array<std::uint32_t, 32> prev_gprs = {};
    std::uint32_t prev_pc, prev_exitcode;

    MachineDataPlotter(mips32::MachineInspector& inspector) noexcept;
    ~MachineDataPlotter();

    void plot() noexcept;
};

void run_io_program(mips32::Machine& machine) noexcept;
void run_debug_sim(mips32::Machine& machine) noexcept;

void load_io_program(mips32::MachineInspector& inspector) noexcept;
void load_debug_program(mips32::MachineInspector& inspector) noexcept;

int main()
{
    using namespace mips32::literals;

    auto iodevice = std::make_unique<ConsoleIODevice>();
    auto filehandler = std::make_unique<CSTDIOFileHandler>();

    mips32::Machine machine{ 512_MB, iodevice.get(), filehandler.get() };
    machine.reset();

    fmt::print("{:^50}\n", "MIPS32 Simulator - Demo");
    fmt::print("1) Program with I/O\n"
               "2) Debugging simulation\n"
               "Choice: ");

    char choice{};
    (void)scanf("%c", &choice);
    switch (choice)
    {
    case '1':
        run_io_program(machine);
        break;

    case '2':
        run_debug_sim(machine);
        break;
    }
}

#pragma region File Handler Implementation
CSTDIOFileHandler::CSTDIOFileHandler() noexcept
{}

CSTDIOFileHandler::~CSTDIOFileHandler()
{
    for (auto* file : files)
        fclose(file);
}

std::uint32_t CSTDIOFileHandler::open(char const* name, char const* flags) noexcept
{
    files.emplace_back(fopen(name, flags));
    return (std::uint32_t)files.size() - 1;
}

std::uint32_t CSTDIOFileHandler::read(std::uint32_t fd, char * dst, std::uint32_t count) noexcept
{
    try
    {
        return (std::uint32_t)fread(dst, 1, count, files.at(fd));
    }
    catch (std::out_of_range const&)
    {
        fprintf(stderr, "Requested to read from a file that doesn't exist!");
        return 0;
    }
}

std::uint32_t CSTDIOFileHandler::write(std::uint32_t fd, char const* src, std::uint32_t count) noexcept
{
    try
    {
        return (std::uint32_t)fwrite(src, 1, count, files[fd]);
    }
    catch (std::out_of_range const&)
    {
        fprintf(stderr, "Requested to write to a file that doesn't exist!");
        return 0;
    }
}

void CSTDIOFileHandler::close(std::uint32_t fd) noexcept
{
    try
    {
        fclose(files[fd]);
    }
    catch (std::out_of_range const&)
    {
        fprintf(stderr, "Requested to close a file that doesn't exist!");
    }
}
#pragma endregion

#pragma region IO Device Implementation
ConsoleIODevice::ConsoleIODevice() noexcept
{
    log = fopen("ConsoleIODevice.log", "w");
    fprintf(log, "%s\n", __FUNCSIG__);
}

ConsoleIODevice::~ConsoleIODevice()
{
    fclose(log);
}

void ConsoleIODevice::print_integer(std::uint32_t value) noexcept
{
    fprintf(log, "%s : %d\n", __FUNCSIG__, value);
    std::cout << (std::int32_t)value;
}

void ConsoleIODevice::print_float(float value) noexcept
{
    fprintf(log, "%s : %.3f\n", __FUNCSIG__, value);
    std::cout << value;
}

void ConsoleIODevice::print_double(double value) noexcept
{
    fprintf(log, "%s : %.3f\n", __FUNCSIG__, value);
    std::cout << value;
}

void ConsoleIODevice::print_string(char const* string) noexcept
{
    fprintf(log, "%s : [%p] first-byte: '%u'\n", __FUNCSIG__, string, (unsigned char)string[0]);
    std::cout << string;
}

void ConsoleIODevice::read_integer(std::uint32_t * value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value);
    std::int32_t v{};
    std::cin >> v;
    fprintf(log, "%s : READ %d\n", __FUNCSIG__, v);
    *value = (std::uint32_t)v;
}

void ConsoleIODevice::read_float(float* value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value);
    float v{};
    std::cin >> v;
    fprintf(log, "%s : READ %f\n", __FUNCSIG__, v);
    *value = v;
}

void ConsoleIODevice::read_double(double* value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value);
    double v{};
    std::cin >> v;
    fprintf(log, "%s : READ %f\n", __FUNCSIG__, v);
    *value = v;
}

void ConsoleIODevice::read_string(char* string, std::uint32_t max_count) noexcept
{
    std::string fmt{};
    fmt += "%";
    fmt += std::to_string(max_count);
    fmt += "c";
    fprintf(log, "%s : [%p] fmt: '%s'\n", __FUNCSIG__, string, fmt.c_str());
    std::string v{};
    std::cin >> v;
    fprintf(log, "%s : READ '%s' with size '%d'\n", __FUNCSIG__, v.c_str(), (int)v.size());
    
    for (std::uint32_t i = 0; i < std::min(max_count, std::uint32_t(v.size())); ++i)
        string[i] = v[i];
}
#pragma endregion

#pragma region Machine Data Plotter Implementation
MachineDataPlotter::MachineDataPlotter(mips32::MachineInspector& inspector) noexcept : inspector(inspector)
{
    prev_pc = inspector.CPU_pc();
    prev_exitcode = inspector.CPU_read_exit_code();

    auto rb = inspector.CPU_gpr_begin();
    auto re = inspector.CPU_gpr_end();
    int i = 0;
    while (rb != re)
        prev_gprs[i++] = *rb++;
}

MachineDataPlotter::~MachineDataPlotter()
{}

/*
  PC: 0x1234'5678 | Exit Code: string
  rN hex int | rM hex int
            ...
*/
void MachineDataPlotter::plot() noexcept
{
    static char const* exit_code[] =
    {
        "NONE",
        "MANUAL_STOP",
        "INTERRUPT",
        "EXCEPTION",
        "EXIT",
    };

    static char const* regs[] = {
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "r8", "r9",
        "r10", "r11", "r12", "r13", "r14",
        "r15", "r16", "r17", "r18", "r19",
        "r20", "r21", "r22", "r23", "r24",
        "r25", "r26", "r27", "r28", "r29",
        "r30", "r31"
    };

    // PC + Exit Code
    {
        auto cur_pc = inspector.CPU_pc();
        auto cur_ec = inspector.CPU_read_exit_code();
        
        fmt::print(" PC{} {:>#10X}{:14}| ", cur_pc != prev_pc ? '<' : ' ', cur_pc, "");
        fmt::print("Exit Code {:>18}\n", exit_code[cur_ec]);
    }
    
    // Prints registers
    {
        int i = 0;
        auto reg = inspector.CPU_gpr_begin();

        while (i < 16)
        {
            auto cur_l = *reg;
            auto cur_r = *(reg + 16);

            bool cur_l_changed = cur_l != prev_gprs[i];
            bool cur_r_changed = cur_r != prev_gprs[i+16];

            fmt::print("{:>3}{:1} {:>#10X} {:>12}", regs[i], cur_l_changed ? '<' : ' ', cur_l, (std::int32_t)cur_l);
            fmt::print(" | {:>3}{:1} {:>#10X} {:>12}\n", regs[i + 16], cur_r_changed ? '<' : ' ', cur_r, (std::int32_t)cur_r);

            ++i;
            ++reg;
        }
    }

    {
        prev_pc = inspector.CPU_pc();
        prev_exitcode = inspector.CPU_read_exit_code();

        auto rb = inspector.CPU_gpr_begin();
        auto re = inspector.CPU_gpr_end();
        int i = 0;
        while (rb != re)
            prev_gprs[i++] = *rb++;
    }
}
#pragma endregion

#pragma region Run I/O Program
void run_io_program(mips32::Machine& machine) noexcept
{
    machine.reset();

    fmt::print("\tI/O Program Simulation\n");
    fmt::print("A simple program that asks for 2 integers and shows simple arithmetic operations.\n");
    fmt::print("Loading executable...\n");

    auto inspector = machine.get_inspector();
    MachineDataPlotter plotter{ inspector };
    load_io_program(inspector);

    fmt::print("Executable loaded!\n");
    fmt::print("Running program...\n\n");

    fmt::print("\tMachine's state before running the I/O executable\n");
    plotter.plot();
    fmt::print("\n");
    machine.start();
    fmt::print("\n\n\tMachine's state after running the I/O executable\n");
    plotter.plot();
}

void load_io_program(mips32::MachineInspector& inspector) noexcept
{
    constexpr std::uint32_t data_segment = 0x0000'0000;
    constexpr std::uint32_t text_segment = 0x8000'0000;

    /**
     * Reads 2 integers and prints x+y, x-y, x*y, x/y
     *
     * -- C-like pseudocode --
     * int x, y;
     * scanf("%d %d", &x, &y);
     * printf("x+y = %d\n"
     *        "x-y = %d\n"
     *        "x*y = %d\n"
     *        "x/y = %d\n"
     *        , x+y
     *        , x-y
     *        , x*y
     *        , x/y);
     * return 0;
     *
     * -- Pseudo-Assembly --
     *
     * .data 0x0000'0000
     * rx:  .asciiz "x = "     # data+0
     * ry:  .asciiz "y = "     # data+5
     * sum: .asciiz "x + y = " # data+10
     * sub: .asciiz "\nx - y = " # data+19
     * mul: .asciiz "\nx * y = " # data+29
     * div: .asciiz "\nx / y = " # data+39
     *
     * .text 0x8000'0000
     * # "x = "
     * addi $a0, $zero, high(rx)  # la $r1, rx
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(rx)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # reads x
     * addi $v0, $zero, 5         # read_integer
     * add  $s0, $zero, $v0
     *
     * # "y = "
     * addi $a0, $zero, high(ry)  # la $a0, ry
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(ry)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # reads y
     * addi $v0, $zero, 5         # read_integer
     * add  $s1, $zero, $v0
     *
     * # "x + y = "
     * addi $a0, $zero, high(sum)  # la $a0, sum
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(sum)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # prints x+y
     * add  $a0, $s0, $s1
     * addi $v0, $zero, 1
     * syscall
     *
     * # "x - y = "
     * addi $a0, $zero, high(sub)  # la $a0, sub
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(sub)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # prints x-y
     * sub  $a0, $s0, $s1
     * addi $v0, $zero, 1
     * syscall
     *
     * # "x * y = "
     * addi $a0, $zero, high(mul)  # la $a0, mul
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(mul)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # prints x*y
     * mul  $a0, $s0, $s1
     * addi $v0, $zero, 1
     * syscall
     *
     * # "x / y = "
     * addi $a0, $zero, high(div)  # la $a0, div
     * sll  $a0, $a0, 16
     * ori  $a0, $a0, low(div)
     * addi $v0, $zero, 4         # print_string
     * syscall
     *
     * # prints x/y
     * div  $a0, $s0, $s1
     * addi $v0, $zero, 1
     * syscall
     */
    constexpr std::uint32_t machine_code[] =
    {
        // $zero = 0
        // $v0 = 2
        // $a0 = 4
        // $s0 = 16
        // $s1 = 17

        // generic print string
        // "ADDIU"_cpu | 4_rd | 0_rs | <addr>,
        // "ADDIU"_cpu | 2_rd | 0_rs | 4,
        // "SYSCALL"_cpu,

        // generic read int
        // "ADDIU"_cpu | 2_rd | 0_rs | 5,
        // "SYSCALL"_cpu,
        // "ADDU"_cpu  | <dest>_rd | 0_rs | 2_rt,

        // x = 
        "ADDIU"_cpu | 4_rt | 0_rs | 0, // data+0
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,
        // $s0 = x
        "ADDIU"_cpu | 2_rt | 0_rs | 5,
        "SYSCALL"_cpu,
        "ADDU"_cpu | 16_rd | 0_rs | 2_rt,

        // y = 
        "ADDIU"_cpu | 4_rt | 0_rs | 5, // data+5
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,
        // $s0 = y
        "ADDIU"_cpu | 2_rt | 0_rs | 5,
        "SYSCALL"_cpu,
        "ADDU"_cpu | 17_rd | 0_rs | 2_rt,

        // x + y = 
        "ADDIU"_cpu | 4_rt | 0_rs | 10, // data+10
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,

        "ADDU"_cpu | 4_rd | 16_rs | 17_rt, // $a0 = $s0 + $s1
        "ADDIU"_cpu | 2_rt | 0_rs | 1,     // print integer
        "SYSCALL"_cpu,

        // x - y = 
        "ADDIU"_cpu | 4_rt | 0_rs | 19, // data+19
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,

        "SUBU"_cpu | 4_rd | 16_rs | 17_rt, // $a0 = $s0 - $s1
        "ADDIU"_cpu | 2_rt | 0_rs | 1,     // print integer
        "SYSCALL"_cpu,

        // x * y = 
        "ADDIU"_cpu | 4_rt | 0_rs | 29, // data+29
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,

        "MULU"_cpu | 4_rd | 16_rs | 17_rt, // $a0 = $s0 * $s1
        "ADDIU"_cpu | 2_rt | 0_rs | 1,     // print integer
        "SYSCALL"_cpu,

        // x / y = 
        "ADDIU"_cpu | 4_rt | 0_rs | 39, // data+39
        "ADDIU"_cpu | 2_rt | 0_rs | 4,
        "SYSCALL"_cpu,

        "DIVU"_cpu | 4_rd | 16_rs | 17_rt, // $a0 = $s0 / $s1
        "ADDIU"_cpu | 2_rt | 0_rs | 1,     // print integer
        "SYSCALL"_cpu,

        "ADDIU"_cpu | 2_rt | 0_rs | 10,   // exit
        "SYSCALL"_cpu,
    };

    /*
     * .data 0x0000'0000
     * rx:.asciiz "x = "        # data + 0
     * ry:.asciiz "y = "        # data + 4
     * sum:.asciiz "x + y = "   # data + 8
     * sub:.asciiz "\nx - y = " # data + 17
     * mul:.asciiz "\nx * y = " # data + 27
     * div:.asciiz "\nx / y = " # data + 36
     */

    char const* _data_str =
        "x = \0"
        "y = \0"
        "x + y = \0"
        "\nx - y = \0"
        "\nx * y = \0"
        "\nx / y = \0";
    constexpr std::uint32_t _data_str_len = 50;

    inspector.RAM_write(data_segment, _data_str, _data_str_len);
    inspector.RAM_write(text_segment, machine_code, std::size(machine_code)*sizeof(std::uint32_t));
    inspector.CPU_pc() = text_segment;
}
#pragma endregion

#pragma region Run Debugging Simulation
void run_debug_sim(mips32::Machine& machine) noexcept
{
    
}

void load_debug_program(mips32::MachineInspector& inspector) noexcept
{}
#pragma endregion