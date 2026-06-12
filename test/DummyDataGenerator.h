#pragma once
#include "model/Sample.h"
#include "model/Order.h"

struct DummyDataGenerator {
    static Sample makeSample(
        std::string id = "S-001",
        std::string name = "테스트시료",
        double avgProductionTime = 1.0,
        double yield = 0.9,
        int stock = 0)
    {
        return Sample{ id, name, avgProductionTime, yield, stock };
    }

    static Order makeOrder(
        std::string id = "O-001",
        std::string sampleId = "S-001",
        std::string customerName = "테스트고객",
        int quantity = 10,
        OrderStatus status = OrderStatus::RESERVED,
        std::string createdAt = "2026-06-12 00:00:00",
        std::string productionStartedAt = "",
        int producedQuantity = 0)
    {
        return Order{ id, sampleId, customerName, quantity, status,
                      createdAt, productionStartedAt, producedQuantity };
    }
};
