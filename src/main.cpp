#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include "repository/SampleRepository.h"
#include "controller/SampleController.h"
#include "view/MainView.h"

int main() {
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    std::filesystem::create_directories("data");

    SampleRepository sampleRepo{ "data/samples.json" };
    SampleController sampleCtrl{ sampleRepo };
    MainView view{ sampleCtrl };
    view.run();

    return 0;
}
