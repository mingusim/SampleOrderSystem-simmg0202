#pragma once
#include "repository/IOrderRepository.h"
#include <string>

class OrderRepository : public IOrderRepository {
public:
    explicit OrderRepository(const std::string& filePath);
    std::optional<Order> findById(const std::string& id) override;
    std::vector<Order> findAll() override;
    void save(const Order& order) override;
    void remove(const std::string& id) override;

private:
    std::string filePath_;
    std::vector<Order> load() const;
    void persist(const std::vector<Order>& orders) const;
};
