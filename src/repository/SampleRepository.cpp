#include "repository/SampleRepository.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

void to_json(json& j, const Sample& s) {
    j = json{
        {"id", s.id},
        {"name", s.name},
        {"avgProductionTime", s.avgProductionTime},
        {"yield", s.yield},
        {"stock", s.stock}
    };
}

void from_json(const json& j, Sample& s) {
    j.at("id").get_to(s.id);
    j.at("name").get_to(s.name);
    j.at("avgProductionTime").get_to(s.avgProductionTime);
    j.at("yield").get_to(s.yield);
    j.at("stock").get_to(s.stock);
}

SampleRepository::SampleRepository(const std::string& filePath)
    : filePath_(filePath) {}

std::vector<Sample> SampleRepository::load() const {
    std::ifstream f(filePath_);
    if (!f.is_open()) return {};
    try {
        return json::parse(f).get<std::vector<Sample>>();
    } catch (...) {
        return {};
    }
}

void SampleRepository::persist(const std::vector<Sample>& samples) const {
    std::filesystem::create_directories(
        std::filesystem::path(filePath_).parent_path());
    std::ofstream f(filePath_);
    f << json(samples).dump(2);
}

std::optional<Sample> SampleRepository::findById(const std::string& id) {
    auto samples = load();
    auto it = std::find_if(samples.begin(), samples.end(),
        [&](const Sample& s) { return s.id == id; });
    if (it == samples.end()) return std::nullopt;
    return *it;
}

std::vector<Sample> SampleRepository::findAll() {
    return load();
}

void SampleRepository::save(const Sample& sample) {
    auto samples = load();
    auto it = std::find_if(samples.begin(), samples.end(),
        [&](const Sample& s) { return s.id == sample.id; });
    if (it != samples.end())
        *it = sample;
    else
        samples.push_back(sample);
    persist(samples);
}

void SampleRepository::remove(const std::string& id) {
    auto samples = load();
    samples.erase(
        std::remove_if(samples.begin(), samples.end(),
            [&](const Sample& s) { return s.id == id; }),
        samples.end());
    persist(samples);
}
