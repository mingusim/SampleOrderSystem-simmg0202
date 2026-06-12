#pragma once
#include <string>

struct Sample {
    std::string id;
    std::string name;
    double avgProductionTime;
    double yield;
    int stock = 0;
};

enum class StockStatus { SURPLUS, SHORTAGE, DEPLETED };

struct SampleStockInfo {
    Sample sample;
    StockStatus stockStatus;
};
