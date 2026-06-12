#pragma once
#include <vector>
#include <optional>
#include <string>
#include "model/Sample.h"

class ISampleRepository {
public:
    virtual ~ISampleRepository() = default;
    virtual std::optional<Sample> findById(const std::string& id) const = 0;
    virtual std::vector<Sample> findAll() const = 0;
    virtual std::vector<Sample> findByName(const std::string& partialName) const = 0;
    virtual void save(const Sample& sample) = 0;
    virtual void remove(const std::string& id) = 0;
};
