#pragma once
#include <vector>
#include <optional>
#include <string>
#include "model/Order.h"

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual std::optional<Order> findById(const std::string& id) = 0;
    virtual std::vector<Order> findAll() = 0;
    virtual void save(const Order& order) = 0;
    virtual void remove(const std::string& id) = 0;
};
