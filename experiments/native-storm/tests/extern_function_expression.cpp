#include "compiler.h"

#include <cassert>
#include <filesystem>

int main(int argc, char **argv)
{
    if (argc != 2) return 2;

    const auto fixture = std::filesystem::absolute(argv[1]);
    COMPILER compiler;
    compiler.SetProgramDirectory(fixture.string().c_str());

    return compiler.CreateProgram("program.c") ? 0 : 1;
}
