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
    MachineDataPlotter() noexcept;
    ~MachineDataPlotter();

    void plot(mips32::MachineInspector& inspector) noexcept;
};

int main()
{
    using namespace mips32::literals;

    auto iodevice = std::make_unique<ConsoleIODevice>();
    auto filehandler = std::make_unique<CSTDIOFileHandler>();

    mips32::Machine machine{ 512_MB, iodevice.get(), filehandler.get() };
    auto inspector = machine.get_inspector();

    MachineDataPlotter plotter{};
    plotter.plot(inspector);
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
MachineDataPlotter::MachineDataPlotter() noexcept
{

}

MachineDataPlotter::~MachineDataPlotter()
{

}

void MachineDataPlotter::plot(mips32::MachineInspector& inspector) noexcept
{
    fmt::print("Testing fmt {}", "hello!");
}
#pragma endregion
