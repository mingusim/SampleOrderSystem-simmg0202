#include "controller/OrderController.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string OrderController::generateOrderId() {
    int maxNum = 0;
    for (const auto& o : orderRepo_.findAll()) {
        if (o.id.size() > 2 && o.id.substr(0, 2) == "O-") {
            try { maxNum = std::max(maxNum, std::stoi(o.id.substr(2))); }
            catch (...) {}
        }
    }
    std::ostringstream oss;
    oss << "O-" << std::setw(3) << std::setfill('0') << (maxNum + 1);
    return oss.str();
}

OrderController::OrderController(ISampleRepository& sampleRepo, IOrderRepository& orderRepo)
    : sampleRepo_(sampleRepo), orderRepo_(orderRepo) {}

bool OrderController::createOrder(const std::string& sampleId,
                                   const std::string& customerName, int quantity) {
    if (!sampleRepo_.findById(sampleId).has_value()) return false;
    Order order;
    order.id                     = generateOrderId();
    order.sampleId               = sampleId;
    order.customerName           = customerName;
    order.quantity               = quantity;
    order.status                 = OrderStatus::RESERVED;
    order.createdAt              = currentTimestamp();
    order.productionStartedAt    = "";
    order.producedQuantity       = 0;
    order.targetProductionQuantity = 0;
    orderRepo_.save(order);
    return true;
}

std::vector<Order> OrderController::getPendingOrders() {
    return orderRepo_.findByStatus(OrderStatus::RESERVED);
}

bool OrderController::approveOrder(const std::string& orderId) {
    auto optOrder = orderRepo_.findById(orderId);
    if (!optOrder) return false;
    Order order = *optOrder;

    auto optSample = sampleRepo_.findById(order.sampleId);
    if (!optSample) return false;
    const Sample& sample = *optSample;

    int reservedQty = 0;
    for (const auto& o : orderRepo_.findBySampleId(order.sampleId)) {
        if ((o.status == OrderStatus::CONFIRMED || o.status == OrderStatus::PRODUCING)
                && o.id != order.id) {
            reservedQty += o.quantity;
        }
    }
    const int available = sample.stock - reservedQty;

    if (available >= order.quantity) {
        order.status = OrderStatus::CONFIRMED;
    } else {
        const int shortage = order.quantity - std::max(0, available);
        order.targetProductionQuantity = static_cast<int>(
            std::ceil(static_cast<double>(shortage) / (sample.yield * 0.9)));
        order.productionStartedAt = currentTimestamp();
        order.status = OrderStatus::PRODUCING;
    }

    orderRepo_.save(order);
    return true;
}

bool OrderController::rejectOrder(const std::string& orderId) {
    auto optOrder = orderRepo_.findById(orderId);
    if (!optOrder) return false;
    Order order = *optOrder;
    order.status = OrderStatus::REJECTED;
    orderRepo_.save(order);
    return true;
}
