#include <cli/parser.hpp>

auto main(int argc, const char* argv[]) -> int {
    jo::git::cli::Parser::init(std::span<const char* const>(argv, argc));
}
