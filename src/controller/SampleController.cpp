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

std::vector<Sample> SampleController::getAllSamples() {
    return repo_.findAll();
}

std::vector<Sample> SampleController::searchByName(const std::string& partialName) {
    return repo_.findByName(partialName);
}
