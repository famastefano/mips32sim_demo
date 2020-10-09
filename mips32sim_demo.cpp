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
#include <string_view>
#include <utility>
#include <algorithm>
#include <sstream>
#include <stack>
#include <stdexcept>

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
    mips32::MachineInspector inspector;
    std::array<std::uint32_t, 32> prev_gprs = {};
    std::uint32_t prev_pc, prev_exitcode;

    MachineDataPlotter(mips32::MachineInspector inspector) noexcept;
    ~MachineDataPlotter();

    void plot() noexcept;
    void plot_reg(std::uint32_t reg) noexcept;
};

struct CommandParser
{
    enum class Command
    {
        INVALID,
        HELP,
        SHOW,
        SINGLE_STEP,
        SET,
        BREAKPOINT,
        RESET,
        EXIT,
        RUN,
    };

    struct Data
    {
        int option;
        std::uint32_t _register, value;
    };

    std::pair<Command, Data> parse_command() noexcept;

    std::stack<std::string> tokens{};
};

struct GDB
{
    FILE* log;
    mips32::Machine& machine;
    MachineDataPlotter plotter;
    std::vector<std::uint32_t> breakpoints;

    GDB(mips32::Machine& machine) noexcept;
    ~GDB();

    void help() noexcept;
    void show(int what, std::uint32_t where) noexcept;
    void single_step() noexcept;
    void set(int what, std::uint32_t where, std::uint32_t value) noexcept;
    void breakpoint(int what, std::uint32_t value) noexcept;
    void reset() noexcept;
    void run() noexcept;
};

void run_io_program(mips32::Machine& machine) noexcept;
void run_debug_sim(mips32::Machine& machine) noexcept;

void load_io_program(mips32::MachineInspector inspector) noexcept;
void load_debug_program(mips32::MachineInspector inspector) noexcept;

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

    std::string choice{'\0'};
    std::getline(std::cin, choice);

    switch (choice[0])
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
    fprintf(log, "%s\n", __FUNCSIG__); fflush(log);
}

ConsoleIODevice::~ConsoleIODevice()
{
    fclose(log);
}

void ConsoleIODevice::print_integer(std::uint32_t value) noexcept
{
    fprintf(log, "%s : %d\n", __FUNCSIG__, value); fflush(log);
    std::cout << (std::int32_t)value;
}

void ConsoleIODevice::print_float(float value) noexcept
{
    fprintf(log, "%s : %.3f\n", __FUNCSIG__, value); fflush(log);
    std::cout << value;
}

void ConsoleIODevice::print_double(double value) noexcept
{
    fprintf(log, "%s : %.3f\n", __FUNCSIG__, value); fflush(log);
    std::cout << value;
}

void ConsoleIODevice::print_string(char const* string) noexcept
{
    fprintf(log, "%s : [%p] first-byte: '%u'\n", __FUNCSIG__, string, (unsigned char)string[0]); fflush(log);
    std::cout << string;
}

void ConsoleIODevice::read_integer(std::uint32_t* value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value); fflush(log);
    std::int32_t v{};
    std::cin >> v;
    fprintf(log, "%s : READ %d\n", __FUNCSIG__, v); fflush(log);
    *value = (std::uint32_t)v;
}

void ConsoleIODevice::read_float(float* value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value); fflush(log);
    float v{};
    std::cin >> v;
    fprintf(log, "%s : READ %f\n", __FUNCSIG__, v); fflush(log);
    *value = v;
}

void ConsoleIODevice::read_double(double* value) noexcept
{
    fprintf(log, "%s : [%p]\n", __FUNCSIG__, value); fflush(log);
    double v{};
    std::cin >> v;
    fprintf(log, "%s : READ %f\n", __FUNCSIG__, v); fflush(log);
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
MachineDataPlotter::MachineDataPlotter(mips32::MachineInspector inspector) noexcept : inspector(inspector)
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
void MachineDataPlotter::plot_reg(std::uint32_t reg) noexcept
{
    static char const* regs[] = {
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "r8", "r9",
        "r10", "r11", "r12", "r13", "r14",
        "r15", "r16", "r17", "r18", "r19",
        "r20", "r21", "r22", "r23", "r24",
        "r25", "r26", "r27", "r28", "r29",
        "r30", "r31"
    };
    auto _reg = *(inspector.CPU_gpr_begin() + reg);
    fmt::print("{:>3} {:>#10X} {:>12}\n", regs[reg], _reg, (std::int32_t)_reg);
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

void load_io_program(mips32::MachineInspector inspector) noexcept
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
    using Command = CommandParser::Command;

    auto inspector = machine.get_inspector();

    GDB gdb{ machine };
    CommandParser cmd_parser{};

    fmt::print("\tGDB-like Simulation\n");
    gdb.help();

    fmt::print("gdb> ");
    auto [command, data] = cmd_parser.parse_command();
    while(command != Command::EXIT || inspector.CPU_read_exit_code() != 0)
    {
        switch (command)
        {
        case Command::INVALID:
        case Command::HELP:
            gdb.help();
            break;

        case Command::SHOW:
            gdb.show(data.option, data._register);
            break;

        case Command::SINGLE_STEP:
            gdb.single_step();
            break;

        case Command::SET:
            gdb.set(data.option, data._register, data.value);
            break;

        case Command::BREAKPOINT:
            gdb.breakpoint(data.option, data.value);
            break;

        case Command::RESET:
            gdb.reset();
            load_debug_program(machine.get_inspector());
            break;

        case Command::RUN:
            gdb.run();
            break;
        }

        {
            fmt::print("gdb> ");
            auto [_command, _data] = cmd_parser.parse_command();
            command = _command;
            data = _data;
        }
    }

    fmt::print("\tMachine's state\n");
    gdb.show(0, 0);
}

void load_debug_program(mips32::MachineInspector inspector) noexcept
{
    load_io_program(inspector);
}
#pragma endregion

#pragma region Command Parser Implementation
CommandParser::Command get_command(std::stack<std::string>& tokens) noexcept;

std::pair<CommandParser::Command, CommandParser::Data> CommandParser::parse_command() noexcept
{
    tokens.swap(std::stack<std::string>{});

    std::string cmdline{};
    std::getline(std::cin, cmdline);

    if (cmdline.empty())
        return { Command::INVALID, {} };

    std::istringstream iss{ std::move(cmdline) };

    auto tolower_str = [](std::string& str) { for (auto& c : str) c = (char)tolower((int)c); };

    {
        std::vector<std::string> toks;
        std::string token{};
        while (iss >> token)
        {
            tolower_str(token);
            toks.emplace_back(std::move(token));
        }
        auto begin = toks.rbegin();
        auto end = toks.rend();
        while (begin != end)
            tokens.push(std::move(*begin++));
    }

    auto command = get_command(tokens);

    switch (command)
    {
    case Command::INVALID:
        return { command, {} };
    case Command::HELP:
    case Command::RESET:
    case Command::EXIT:
    case Command::SINGLE_STEP:
    case Command::RUN:
    {
        if (!tokens.empty())
            return{ Command::INVALID, {} };

        return { command, {} };
    }

    case Command::SHOW:
    {
        if (tokens.size() != 1)
            return { Command::INVALID, {} };

        auto tok = tokens.top();
        if (tok == "state")
        {
            Data d{};
            d.option = 0;
            return { command, d };
        }

        static std::array<std::string, 32> regs
        {
            "r0", "r1", "r2", "r3", "r4",
            "r5", "r6", "r7", "r8", "r9",
            "r10", "r11", "r12", "r13", "r14",
            "r15", "r16", "r17", "r18", "r19",
            "r20", "r21", "r22", "r23", "r24",
            "r25", "r26", "r27", "r28", "r29",
            "r30", "r31"
        };

        auto r = std::find(regs.cbegin(), regs.cend(), tok);
        if (r == regs.cend())
            return { Command::INVALID,{} };

        Data d{};
        d.option = 1;
        d._register = r - regs.cbegin();

        return { command, d };
    }
    case Command::SET:
    {
        if (tokens.size() != 2)
            return { Command::INVALID, {} };

        static std::array<std::string, 32> regs
        {
            "r0", "r1", "r2", "r3", "r4",
            "r5", "r6", "r7", "r8", "r9",
            "r10", "r11", "r12", "r13", "r14",
            "r15", "r16", "r17", "r18", "r19",
            "r20", "r21", "r22", "r23", "r24",
            "r25", "r26", "r27", "r28", "r29",
            "r30", "r31"
        };

        auto tok_reg = tokens.top(); tokens.pop();
        auto tok_val = tokens.top(); tokens.pop();

        auto r = std::find(regs.cbegin(), regs.cend(), tok_reg);
        if (r == regs.cend())
            return { Command::INVALID,{} };

        try
        {
            Data d{};
            d.option = 0;
            d._register = r - regs.cbegin();
            d.value = std::stoul(tok_val);

            return { command, d };
        }
        catch (std::exception const& e)
        {
            return { Command::INVALID, {} };
        }
    }
    case Command::BREAKPOINT:
    {
        if (tokens.empty())
            return { Command::INVALID, {} };

        auto opt_tok = tokens.top(); tokens.pop();

        if(opt_tok == "clear")
        {
            Data d{};
            d.option = 0;
            return { command, d };
        }
        else if (opt_tok == "list")
        {
            Data d{};
            d.option = 1;
            return { command, d };
        }
        else if (opt_tok == "pc")
        {
            if (tokens.empty())
                return { Command::INVALID, {} };

            try
            {
                auto tok_val = tokens.top(); tokens.pop();

                Data d{};
                d.option = 2;
                d.value = std::stoul(tok_val);

                return { command, d };
            }
            catch (std::exception const& e)
            {
                return { Command::INVALID, {} };
            }
        }

        return { Command::INVALID, {} };
    }
    }

    return { Command::INVALID, {} };
}

CommandParser::Command get_command(std::stack<std::string>& tokens) noexcept
{
    using Command = CommandParser::Command;

    #define HASHED_STR(x) std::hash<std::string_view>{}(x)
    static std::array<std::size_t, 8> command_hash
    {
        HASHED_STR("help"),
        HASHED_STR("show"),
        HASHED_STR("si"),
        HASHED_STR("set"),
        HASHED_STR("bp"),
        HASHED_STR("reset"),
        HASHED_STR("exit"),
        HASHED_STR("run"),
    };
    #undef HASHED_STR

    static std::array<Command, 8> command_type
    {
        Command::HELP,
        Command::SHOW,
        Command::SINGLE_STEP,
        Command::SET,
        Command::BREAKPOINT,
        Command::RESET,
        Command::EXIT,
        Command::RUN,
    };

    auto c = std::find(command_hash.cbegin(), command_hash.cend(), std::hash<std::string>{}(tokens.top()));
    if (c == command_hash.cend())
        return Command::INVALID;

    tokens.pop();
    return command_type[c - command_hash.cbegin()];
}
#pragma endregion

#pragma region GDB Implementation
GDB::GDB(mips32::Machine& machine) noexcept : machine(machine), plotter(machine.get_inspector())
{
    log = fopen("GDB.log", "w");
    fprintf(log, __FUNCSIG__"\n"); fflush(log);
}

GDB::~GDB()
{
    fclose(log);
}

void GDB::help() noexcept
{
    fprintf(log, __FUNCSIG__ "\n"); fflush(log);

    fmt::print("\nUsage: gdb> help|show|bp|set|si|run|reset|exit\n"
               "help\n\tPrints this message.\n"
               "show state\n\tShows the CPU's state.\n"
               "show <reg>\n\tShows the content of the specified register.\n"
               "bp list\n\tLists all the breakpoints.\n"
               "bp clear\n\tDeletes all the breakpoints.\n"
               "bp pc <addr>\n\tPause the execution when the PC equals to <addr>.\n"
               "set <reg> <value>\n\tSets the content of the specified register <reg> to <value>.\n"
               "si\n\tExecute 1 instruction.\n"
               "run\n\tRuns the program until a breakpoint is hit or it terminates.\n"
               "reset\n\tResets the Machine.\n"
               "exit\n\tTerminates GDB.\n"
    );
}

void GDB::show(int what, std::uint32_t where) noexcept
{
    fprintf(log, __FUNCSIG__ " what: %d, where: %lu\n", what, where); fflush(log);

    switch (what)
    {
    case 0: // state
        plotter.plot();
        break;
    case 1: // reg
        plotter.plot_reg(where);
        break;
    default:
        help();
        break;
    }
}

void GDB::single_step() noexcept
{
    fprintf(log, __FUNCSIG__ "\n"); fflush(log);

    machine.single_step();
}

void GDB::set(int what, std::uint32_t where, std::uint32_t value) noexcept
{
    fprintf(log, __FUNCSIG__ " what: %d, where: %lu, value: %lu\n", what, where, value); fflush(log);

    switch (what)
    {
    case 0: // reg
        *(machine.get_inspector().CPU_gpr_begin() + where) = value;
        break;
    default:
        help();
        break;
    }
}

void GDB::breakpoint(int what, std::uint32_t value) noexcept
{
    fprintf(log, __FUNCSIG__ " what: %d, value: %lu\n", what, value); fflush(log);

    switch (what)
    {
    case 0: // clear
        breakpoints.clear();
        break;
    case 1: // list
    {
        if (!breakpoints.size())
        {
            fmt::print("No breakpoint set\n");
        }
        else
        {
            fmt::print("Breakpoint(s) will trigger at the following PC values:\n");
            for (auto bp : breakpoints)
                fmt::print("{:10X}\n", bp);
        }

        break;
    }
    case 2: // pc
        breakpoints.emplace_back(value);
        break;
    default:
        break;
    }
}

void GDB::reset() noexcept
{
    fprintf(log, __FUNCSIG__ "\n"); fflush(log);
    machine.reset();
}

void GDB::run() noexcept
{
    fprintf(log, __FUNCSIG__ "\n"); fflush(log);
    if (!breakpoints.size())
    {
        machine.start();
    }
    else
    {
        auto bp = [this]() -> bool
        {
            return std::find(breakpoints.cbegin(), breakpoints.cend(), machine.get_inspector().CPU_pc()) != breakpoints.cend();
        };

        while (!bp())
            machine.single_step();

        fmt::print("\nBreakpoint hit at [{:X}]\n", machine.get_inspector().CPU_pc());
    }
}
#pragma endregion