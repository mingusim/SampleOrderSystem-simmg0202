#pragma once
#include "model/Order.h"
#include "repository/ISampleRepository.h"
#include "repository/IOrderRepository.h"
#include <string>
#include <vector>

class OrderController {
public:
    explicit OrderController(ISampleRepository& sampleRepo, IOrderRepository& orderRepo);
    bool createOrder(const std::string& sampleId, const std::string& customerName, int quantity);
    std::vector<Order> getPendingOrders();
    bool approveOrder(const std::string& orderId);
    bool rejectOrder(const std::string& orderId);

private:
    ISampleRepository& sampleRepo_;
    IOrderRepository&  orderRepo_;
};
