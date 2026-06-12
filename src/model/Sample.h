#pragma once
#include <string>

struct Sample {
    std::string id;
    std::string name;
    double avgProductionTime;
    double yield;
    int stock = 0;
};
