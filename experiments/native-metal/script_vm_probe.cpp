// Execute an isolated script fixture with the real compiler and save codec.
#include "core_impl.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <spdlog/spdlog.h>

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 5) return 64;
    const std::regex identifier("[A-Za-z_][A-Za-z_0-9]*");
    if (!std::regex_match(argv[2], identifier) ||
        (argc > 4 && !std::regex_match(argv[4], identifier))) return 64;
    const int rounds = argc > 3 ? std::stoi(argv[3]) : 0;
    if (rounds < 0 || rounds > 4) return 64;
    const auto root = std::filesystem::canonical(argv[1]);
    if (!std::filesystem::is_regular_file(root / "cases.c")) return 64;
    std::filesystem::current_path(root);
    const auto userdata = root / ".vm-userdata";
    std::filesystem::create_directories(userdata / "Logs");
    if (setenv("STORM_USERDATA", userdata.c_str(), 1) != 0) return 64;
    core_internal.Init();
    core_internal.InitBase();
    core_internal.Controls = new CONTROLS;
    auto &vm = *core_internal.Compiler;
    vm.SetProgramDirectory(".");
    {
        std::ofstream driver("driver.c");
        driver << "#include \"cases.c\"\nint RunnerResult;\nvoid Main(){RunnerResult="
               << argv[2] << "();}\n";
        if (argc > 4) driver << "void RunnerAfterLoad(){RunnerResult=" << argv[4] << "();}\n";
        if (!driver) return 64;
    }
    if (!vm.CreateProgram("driver.c")) {
        spdlog::apply_all([](auto logger) { logger->flush(); });
        return 65;
    }
    auto result = [&](const char *name) {
        auto *data = static_cast<VDATA *>(core_internal.GetScriptVariable("RunnerResult"));
        int32_t value = 0;
        if (!data || !data->Get(value)) return false;
        std::cout << name << "=" << value << "\n";
        return value == 0;
    };
    vm.Run();
    bool okay = result(argv[2]);
    for (int i = 0; okay && i < rounds; ++i) {
        const auto state = userdata / "round.bin";
        { std::fstream out(state, std::ios::binary | std::ios::out | std::ios::trunc);
          if (!out || !vm.SaveState(out)) return 66; }
        { std::fstream in(state, std::ios::binary | std::ios::in);
          if (!in || !vm.LoadState(in)) return 67; }
        if (argc > 4) {
            vm.SetEventHandler("runner_after_load", "RunnerAfterLoad", 0);
            vm.ProcessEvent("runner_after_load");
            vm.DelEventHandler("runner_after_load", "RunnerAfterLoad");
            okay = result(argv[4]);
        }
    }
    spdlog::apply_all([](auto logger) { logger->flush(); });
    const auto errors = userdata / "Logs/error.log";
    const auto bytes = std::filesystem::exists(errors) ? std::filesystem::file_size(errors) : 0;
    std::cout << "script_errors=" << bytes << "\n";
    return okay && bytes == 0 ? 0 : 1;
}
