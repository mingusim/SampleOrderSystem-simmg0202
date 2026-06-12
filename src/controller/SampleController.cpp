#include "controller/SampleController.h"

SampleController::SampleController(ISampleRepository& repo)
    : repo_(repo) {}

bool SampleController::registerSample(const std::string& id, const std::string& name,
                                       double avgProductionTime, double yield) {
    if (yield < 0.01 || yield > 1.0) return false;
    if (repo_.findById(id).has_value()) return false;
    repo_.save(Sample{ id, name, avgProductionTime, yield, 0 });
    return true;
}

std::optional<Sample> SampleController::findById(const std::string& id) {
    return repo_.findById(id);
}

std::vector<Sample> SampleController::getAllSamples() {
    return repo_.findAll();
}

std::vector<Sample> SampleController::searchByName(const std::string& partialName) {
    return repo_.findByName(partialName);
}

std::vector<SampleStockInfo> SampleController::getStockStatus(const std::vector<Order>& activeOrders) {
    std::vector<SampleStockInfo> result;
    for (const auto& sample : repo_.findAll()) {
        int activeQty = 0;
        for (const auto& o : activeOrders) {
            if (o.sampleId == sample.id) activeQty += o.quantity;
        }
        StockStatus status;
        if (sample.stock == 0)
            status = StockStatus::DEPLETED;
        else if (sample.stock < activeQty)
            status = StockStatus::SHORTAGE;
        else
            status = StockStatus::SURPLUS;
        result.push_back({sample, status});
    }
    return result;
}
