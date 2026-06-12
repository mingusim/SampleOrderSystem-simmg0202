#pragma once
#include "repository/ISampleRepository.h"
#include <optional>
#include <string>
#include <vector>

class SampleController {
public:
    explicit SampleController(ISampleRepository& repo);
    bool registerSample(const std::string& id, const std::string& name,
                        double avgProductionTime, double yield);
    std::optional<Sample> findById(const std::string& id);
    std::vector<Sample> getAllSamples();
    std::vector<Sample> searchByName(const std::string& partialName);

private:
    ISampleRepository& repo_;
};
