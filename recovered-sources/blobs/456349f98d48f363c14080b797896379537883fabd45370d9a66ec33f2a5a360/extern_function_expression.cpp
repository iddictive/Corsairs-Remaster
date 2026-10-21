#include "compiler.h"

#include <cassert>
#include <filesystem>

int main(int argc, char **argv)
{
    assert(argc == 2);

    const auto fixture = std::filesystem::absolute(argv[1]);
    COMPILER compiler;
    compiler.SetProgramDirectory(fixture.string().c_str());

    assert(compiler.CreateProgram("program.c"));
    return 0;
}
