#pragma once
#include "controller/SampleController.h"

class MainView {
public:
    explicit MainView(SampleController& sampleController);
    void run();

private:
    SampleController& sampleCtrl_;

    static int readMenuChoice();
    void showMainMenu();
    void handleSampleMenu();
    void printSampleList(const std::vector<Sample>& samples);
};
