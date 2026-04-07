#pragma once
#include <string>
#include <vector>

namespace GOW {

class CliApp {
public:
    static int Run(int argc, char** argv);

private:
    static void PrintHelp();
    static int HandleExtract(const std::vector<std::string>& args);
    static int HandleInspect(const std::vector<std::string>& args);
    static int HandleParseWad(const std::vector<std::string>& args);
};

} // namespace GOW
