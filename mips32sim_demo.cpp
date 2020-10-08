#include "mips32/test/helpers/test_cpu_instructions.hpp"

#include <mips32/machine.hpp>
#include <mips32/literals.hpp>

#include <fmt/format.h>

#include <vector>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <memory>

struct ConsoleIODevice final : virtual public mips32::IODevice
{
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

void run_io_program(MachineDataPlotter& plotter) noexcept;
void run_debug_sim(MachineDataPlotter& plotter) noexcept;

int main()
{
    using namespace mips32::literals;

    auto iodevice = std::make_unique<ConsoleIODevice>();
    auto filehandler = std::make_unique<CSTDIOFileHandler>();

    mips32::Machine machine{ 512_MB, iodevice.get(), filehandler.get() };
    machine.reset();

    auto inspector = machine.get_inspector();
    MachineDataPlotter plotter{inspector};

    fmt::print("{:^50}\n", "MIPS32 Simulator - Demo");
    fmt::print("1) Program with I/O\n"
               "2) Debugging simulation\n"
               "Choice: ");

    char choice{};
    (void)scanf("%c", &choice);
    switch (choice)
    {
    case '1': run_io_program(plotter); break;
    case '2': run_debug_sim(plotter); break;
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
{}

ConsoleIODevice::~ConsoleIODevice()
{}

void ConsoleIODevice::print_integer(std::uint32_t value) noexcept
{
    printf("%lld", (std::int64_t)value);
}

void ConsoleIODevice::print_float(float value) noexcept
{
    printf("%.3f", value);
}

void ConsoleIODevice::print_double(double value) noexcept
{
    printf("%.3f", value);
}

void ConsoleIODevice::print_string(char const* string) noexcept
{
    printf("%s", string);
}

void ConsoleIODevice::read_integer(std::uint32_t * value) noexcept
{
    (void)scanf("%d", value);
}

void ConsoleIODevice::read_float(float* value) noexcept
{
    (void)scanf("%f", value);
}

void ConsoleIODevice::read_double(double* value) noexcept
{
    (void)scanf("%lf", value);
}

void ConsoleIODevice::read_string(char* string, std::uint32_t max_count) noexcept
{
    std::string fmt{};
    fmt += "%";
    fmt += std::to_string(max_count);
    fmt += "c";
    (void)scanf(fmt.c_str(), string);
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
        fmt::print("Exit Code {:>18}\n", exit_code[1]);
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
void run_io_program(MachineDataPlotter& plotter) noexcept
{
    fmt::print("\n\n{:^50}\n", "Program Info");
    fmt::print("<<info placeholder>>\n");
    fmt::print("\n{:^50}\n", "CPU's State Before Execution");
    plotter.plot();

    fmt::print("\n{:^50}\n", "CPU's State After Execution");
    plotter.plot();
}
#pragma endregion

#pragma region Run Debugging Simulation
void run_debug_sim(MachineDataPlotter& plotter) noexcept
{

}
#pragma endregion