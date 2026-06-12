#include "controller/OrderController.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

static constexpr std::string_view kOrderIdPrefix = "O-";

static bool orderByProductionStart(const Order& a, const Order& b) {
    return a.productionStartedAt < b.productionStartedAt;
}

static std::vector<Order> sortedProducing(std::vector<Order> orders) {
    std::sort(orders.begin(), orders.end(), orderByProductionStart);
    return orders;
}

std::string OrderController::generateOrderId() {
    int maxNum = 0;
    for (const auto& o : orderRepo_.findAll()) {
        if (o.id.size() > kOrderIdPrefix.size() &&
                o.id.substr(0, kOrderIdPrefix.size()) == kOrderIdPrefix) {
            try { maxNum = std::max(maxNum, std::stoi(o.id.substr(kOrderIdPrefix.size()))); }
            catch (...) {}
        }
    }
    std::ostringstream oss;
    oss << kOrderIdPrefix << std::setw(3) << std::setfill('0') << (maxNum + 1);
    return oss.str();
}

int OrderController::calcReservedQuantity(const std::string& sampleId,
                                           const std::string& excludeOrderId) const {
    int qty = 0;
    for (const auto& o : orderRepo_.findBySampleId(sampleId)) {
        if ((o.status == OrderStatus::CONFIRMED || o.status == OrderStatus::PRODUCING)
                && o.id != excludeOrderId) {
            qty += o.quantity;
        }
    }
    return qty;
}

void OrderController::transitionToProducing(Order& order, const Sample& sample, int available) {
    const int shortage = order.quantity - std::max(0, available);
    order.targetProductionQuantity = static_cast<int>(
        std::ceil(static_cast<double>(shortage) / (sample.yield * 0.9)));
    order.productionStartedAt = TimeUtils::currentTimestamp();
    order.status = OrderStatus::PRODUCING;
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
    order.createdAt              = TimeUtils::currentTimestamp();
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

    const int available = sample.stock - calcReservedQuantity(order.sampleId, order.id);

    if (available >= order.quantity)
        order.status = OrderStatus::CONFIRMED;
    else
        transitionToProducing(order, sample, available);

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

OrderStats OrderController::getOrderStats() {
    OrderStats stats;
    for (const auto& o : orderRepo_.findAll()) {
        switch (o.status) {
            case OrderStatus::RESERVED:  stats.reserved++;  break;
            case OrderStatus::PRODUCING: stats.producing++; break;
            case OrderStatus::CONFIRMED: stats.confirmed++; break;
            case OrderStatus::RELEASE:   stats.release++;   break;
            case OrderStatus::REJECTED:  break;  // 모니터링 제외
        }
    }
    return stats;
}

std::vector<Order> OrderController::getActiveOrders() {
    auto confirmed = orderRepo_.findByStatus(OrderStatus::CONFIRMED);
    auto producing = orderRepo_.findByStatus(OrderStatus::PRODUCING);
    confirmed.insert(confirmed.end(), producing.begin(), producing.end());
    return confirmed;
}

std::unordered_map<std::string, int> OrderController::getActiveQtyBySample() {
    std::unordered_map<std::string, int> result;
    for (const auto& o : getActiveOrders())
        result[o.sampleId] += o.quantity;
    return result;
}

void OrderController::updateProduction(const std::string& now) {
    auto producing = sortedProducing(orderRepo_.findByStatus(OrderStatus::PRODUCING));
    if (producing.empty()) return;

    // FIFO 단일 생산라인: 가장 먼저 시작된 주문만 처리
    Order order = producing.front();
    auto optSample = sampleRepo_.findById(order.sampleId);
    if (!optSample) return;
    Sample sample = *optSample;

    const double elapsed = TimeUtils::elapsedHours(order.productionStartedAt, now);
    const int cumulative = static_cast<int>(std::ceil(elapsed / sample.avgProductionTime));
    const int totalProduced = std::min(cumulative, order.targetProductionQuantity);
    const int delta = totalProduced - order.producedQuantity;

    if (delta <= 0) return;

    order.producedQuantity += delta;
    sample.stock += delta;

    if (order.producedQuantity >= order.targetProductionQuantity) {
        order.status = OrderStatus::CONFIRMED;
        orderRepo_.save(order);
        sampleRepo_.save(sample);

        // 다음 대기 주문이 있으면 실제 생산 시작 시각으로 갱신
        if (producing.size() > 1) {
            Order next = producing[1];
            next.productionStartedAt = now;
            orderRepo_.save(next);
        }
    } else {
        orderRepo_.save(order);
        sampleRepo_.save(sample);
    }
}

std::optional<ProductionInfo> OrderController::makeProductionInfo(const Order& order) const {
    auto optSample = sampleRepo_.findById(order.sampleId);
    if (!optSample) return std::nullopt;
    const Sample& sample = *optSample;
    return ProductionInfo{
        order, sample,
        sample.avgProductionTime * order.targetProductionQuantity
    };
}

std::optional<ProductionInfo> OrderController::getCurrentProduction() {
    const auto producing = sortedProducing(orderRepo_.findByStatus(OrderStatus::PRODUCING));
    if (producing.empty()) return std::nullopt;
    return makeProductionInfo(producing.front());
}

std::vector<ProductionInfo> OrderController::getProductionQueue() {
    const auto producing = sortedProducing(orderRepo_.findByStatus(OrderStatus::PRODUCING));

    std::vector<ProductionInfo> result;
    // front(index 0)는 현재 생산 중 — 대기 큐에서 제외
    for (auto it = std::next(producing.begin()); it != producing.end(); ++it) {
        if (auto info = makeProductionInfo(*it))
            result.push_back(std::move(*info));
    }
    return result;
}

std::vector<Order> OrderController::getConfirmedOrders() {
    return orderRepo_.findByStatus(OrderStatus::CONFIRMED);
}

bool OrderController::releaseOrder(const std::string& orderId) {
    auto optOrder = orderRepo_.findById(orderId);
    if (!optOrder) return false;
    Order order = *optOrder;
    if (order.status != OrderStatus::CONFIRMED) return false;

    auto optSample = sampleRepo_.findById(order.sampleId);
    if (!optSample) return false;
    Sample sample = *optSample;

    order.status = OrderStatus::RELEASE;
    sample.stock -= order.quantity;

    orderRepo_.save(order);
    sampleRepo_.save(sample);
    return true;
}
