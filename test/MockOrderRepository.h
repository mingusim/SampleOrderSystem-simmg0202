#pragma once
#include <gmock/gmock.h>
#include "repository/IOrderRepository.h"

class MockOrderRepository : public IOrderRepository {
public:
    MOCK_METHOD(std::optional<Order>, findById, (const std::string& id), (const, override));
    MOCK_METHOD(std::vector<Order>, findAll, (), (const, override));
    MOCK_METHOD(std::vector<Order>, findByStatus, (OrderStatus status), (const, override));
    MOCK_METHOD(std::vector<Order>, findBySampleId, (const std::string& sampleId), (const, override));
    MOCK_METHOD(void, save, (const Order& order), (override));
    MOCK_METHOD(void, remove, (const std::string& id), (override));
};
