#pragma once
#include "model/Order.h"
#include "repository/ISampleRepository.h"
#include "repository/IOrderRepository.h"
#include <string>
#include <unordered_map>
#include <vector>

class OrderController {
public:
    explicit OrderController(ISampleRepository& sampleRepo, IOrderRepository& orderRepo);
    bool createOrder(const std::string& sampleId, const std::string& customerName, int quantity);
    std::vector<Order> getPendingOrders();
    bool approveOrder(const std::string& orderId);
    bool rejectOrder(const std::string& orderId);
    OrderStats getOrderStats();
    std::vector<Order> getActiveOrders();
    std::unordered_map<std::string, int> getActiveQtyBySample();

private:
    ISampleRepository& sampleRepo_;
    IOrderRepository&  orderRepo_;

    std::string generateOrderId();
    int calcReservedQuantity(const std::string& sampleId, const std::string& excludeOrderId) const;
    void transitionToProducing(Order& order, const Sample& sample, int available);
};
