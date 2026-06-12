#pragma once
#include "repository/ISampleRepository.h"
#include <string>

class SampleRepository : public ISampleRepository {
public:
    explicit SampleRepository(const std::string& filePath);
    std::optional<Sample> findById(const std::string& id) const override;
    std::vector<Sample> findAll() const override;
    std::vector<Sample> findByName(const std::string& partialName) const override;
    void save(const Sample& sample) override;
    void remove(const std::string& id) override;

private:
    std::string filePath_;
    std::vector<Sample> load() const;
    void persist(const std::vector<Sample>& samples) const;
};
