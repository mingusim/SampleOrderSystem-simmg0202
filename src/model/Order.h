#pragma once
#include <string>

enum class OrderStatus {
    RESERVED,
    REJECTED,
    PRODUCING,
    CONFIRMED,
    RELEASE
};

struct Order {
    std::string id;
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;
    std::string createdAt;
    std::string productionStartedAt;
    int producedQuantity = 0;
    int targetProductionQuantity = 0;
};
