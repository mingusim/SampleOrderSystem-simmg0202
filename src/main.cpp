#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>

int main() {
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    std::filesystem::create_directories("data");
    return 0;
}
