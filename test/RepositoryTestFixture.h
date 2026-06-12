#pragma once
#include <gtest/gtest.h>
#include <filesystem>
#include <string>

class RepositoryTestFixture : public ::testing::Test {
protected:
    std::string testDataDir_ = "test_data_temp";

    void SetUp() override {
        std::filesystem::create_directories(testDataDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDataDir_);
    }
};
