#pragma once
#include <gmock/gmock.h>
#include "repository/ISampleRepository.h"

class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(std::optional<Sample>, findById, (const std::string& id), (override));
    MOCK_METHOD(std::vector<Sample>, findAll, (), (override));
    MOCK_METHOD(std::vector<Sample>, findByName, (const std::string& partialName), (override));
    MOCK_METHOD(void, save, (const Sample& sample), (override));
    MOCK_METHOD(void, remove, (const std::string& id), (override));
};
