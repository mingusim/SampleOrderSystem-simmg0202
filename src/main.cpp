#ifdef _WIN32
#include <windows.h>
#endif
#include <filesystem>
#include "repository/SampleRepository.h"
#include "repository/OrderRepository.h"
#include "controller/SampleController.h"
#include "controller/OrderController.h"
#include "view/MainView.h"

int main() {
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
    std::filesystem::create_directories("data");

    SampleRepository sampleRepo{ "data/samples.json" };
    OrderRepository  orderRepo { "data/orders.json"  };
    SampleController sampleCtrl{ sampleRepo };
    OrderController  orderCtrl { sampleRepo, orderRepo };
    MainView view{ sampleCtrl, orderCtrl };
    view.run();

    return 0;
}
