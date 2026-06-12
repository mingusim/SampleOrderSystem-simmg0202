#include "repository/OrderRepository.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

static std::string statusToString(OrderStatus s) {
    switch (s) {
        case OrderStatus::RESERVED:  return "RESERVED";
        case OrderStatus::REJECTED:  return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE:   return "RELEASE";
        default:                     return "RESERVED";
    }
}

static OrderStatus statusFromString(const std::string& s) {
    if (s == "REJECTED")  return OrderStatus::REJECTED;
    if (s == "PRODUCING") return OrderStatus::PRODUCING;
    if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (s == "RELEASE")   return OrderStatus::RELEASE;
    return OrderStatus::RESERVED;
}

void to_json(json& j, const Order& o) {
    j = json{
        {"id", o.id},
        {"sampleId", o.sampleId},
        {"customerName", o.customerName},
        {"quantity", o.quantity},
        {"status", statusToString(o.status)},
        {"createdAt", o.createdAt},
        {"productionStartedAt", o.productionStartedAt},
        {"producedQuantity", o.producedQuantity}
    };
}

void from_json(const json& j, Order& o) {
    j.at("id").get_to(o.id);
    j.at("sampleId").get_to(o.sampleId);
    j.at("customerName").get_to(o.customerName);
    j.at("quantity").get_to(o.quantity);
    o.status = statusFromString(j.at("status").get<std::string>());
    j.at("createdAt").get_to(o.createdAt);
    j.at("productionStartedAt").get_to(o.productionStartedAt);
    j.at("producedQuantity").get_to(o.producedQuantity);
}

OrderRepository::OrderRepository(const std::string& filePath)
    : filePath_(filePath) {}

std::vector<Order> OrderRepository::load() const {
    std::ifstream f(filePath_);
    if (!f.is_open()) return {};
    try {
        return json::parse(f).get<std::vector<Order>>();
    } catch (...) {
        return {};
    }
}

void OrderRepository::persist(const std::vector<Order>& orders) const {
    std::filesystem::create_directories(
        std::filesystem::path(filePath_).parent_path());
    std::ofstream f(filePath_);
    f << json(orders).dump(2);
}

std::optional<Order> OrderRepository::findById(const std::string& id) {
    auto orders = load();
    auto it = std::find_if(orders.begin(), orders.end(),
        [&](const Order& o) { return o.id == id; });
    if (it == orders.end()) return std::nullopt;
    return *it;
}

std::vector<Order> OrderRepository::findAll() {
    return load();
}

void OrderRepository::save(const Order& order) {
    auto orders = load();
    auto it = std::find_if(orders.begin(), orders.end(),
        [&](const Order& o) { return o.id == order.id; });
    if (it != orders.end())
        *it = order;
    else
        orders.push_back(order);
    persist(orders);
}

void OrderRepository::remove(const std::string& id) {
    auto orders = load();
    orders.erase(
        std::remove_if(orders.begin(), orders.end(),
            [&](const Order& o) { return o.id == id; }),
        orders.end());
    persist(orders);
}
