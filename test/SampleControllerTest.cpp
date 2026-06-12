#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MockSampleRepository.h"
#include "controller/SampleController.h"

using ::testing::Return;
using ::testing::Field;
using ::testing::_;

class SampleControllerTest : public ::testing::Test {
protected:
    MockSampleRepository mockRepo_;
    SampleController controller_{ mockRepo_ };
};

// FR-010: 수율 범위 검증은 중복 체크보다 먼저 수행 (findById 호출 없어야 함)
TEST_F(SampleControllerTest, Register_YieldTooLow_ReturnsFalse) {
    EXPECT_CALL(mockRepo_, findById(_)).Times(0);
    EXPECT_FALSE(controller_.registerSample("S-001", "시료", 1.0, 0.009));
}

TEST_F(SampleControllerTest, Register_YieldTooHigh_ReturnsFalse) {
    EXPECT_CALL(mockRepo_, findById(_)).Times(0);
    EXPECT_FALSE(controller_.registerSample("S-001", "시료", 1.0, 1.001));
}

TEST_F(SampleControllerTest, Register_YieldBoundary_Low_Succeeds) {
    EXPECT_CALL(mockRepo_, findById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(mockRepo_, save(_)).Times(1);
    EXPECT_TRUE(controller_.registerSample("S-001", "시료", 1.0, 0.01));
}

TEST_F(SampleControllerTest, Register_YieldBoundary_High_Succeeds) {
    EXPECT_CALL(mockRepo_, findById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(mockRepo_, save(_)).Times(1);
    EXPECT_TRUE(controller_.registerSample("S-001", "시료", 1.0, 1.0));
}

// FR-010: ID 중복 시 등록 거부
TEST_F(SampleControllerTest, Register_DuplicateId_ReturnsFalse) {
    Sample existing{ "S-001", "기존시료", 1.0, 0.9, 0 };
    EXPECT_CALL(mockRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{ existing }));
    EXPECT_CALL(mockRepo_, save(_)).Times(0);
    EXPECT_FALSE(controller_.registerSample("S-001", "새시료", 1.0, 0.9));
}

// FR-010: 정상 등록 → save 1회 호출, stock 초기값 0
TEST_F(SampleControllerTest, Register_Success_CallsSaveOnce) {
    EXPECT_CALL(mockRepo_, findById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(mockRepo_, save(_)).Times(1);
    EXPECT_TRUE(controller_.registerSample("S-001", "새시료", 1.0, 0.9));
}

TEST_F(SampleControllerTest, Register_Success_StockIsZero) {
    EXPECT_CALL(mockRepo_, findById("S-001")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(mockRepo_, save(Field(&Sample::stock, 0))).Times(1);
    controller_.registerSample("S-001", "새시료", 1.0, 0.9);
}

// FR-011: 전체 목록 조회
TEST_F(SampleControllerTest, GetAllSamples_ReturnsFindAllResult) {
    std::vector<Sample> samples = { {"S-001", "시료1", 1.0, 0.9, 10} };
    EXPECT_CALL(mockRepo_, findAll()).WillOnce(Return(samples));
    auto result = controller_.getAllSamples();
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ("S-001", result[0].id);
}

// FR-012: 이름 부분 일치 검색
TEST_F(SampleControllerTest, SearchByName_DelegatesToRepository) {
    std::vector<Sample> matches = { {"S-001", "반도체시료", 1.0, 0.9, 0} };
    EXPECT_CALL(mockRepo_, findByName("반도체")).WillOnce(Return(matches));
    auto result = controller_.searchByName("반도체");
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ("S-001", result[0].id);
}

// FR-012: ID 검색 — 존재하는 경우
TEST_F(SampleControllerTest, FindById_Found_ReturnsSample) {
    Sample s{ "S-001", "반도체시료", 1.0, 0.9, 10 };
    EXPECT_CALL(mockRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{ s }));
    auto result = controller_.findById("S-001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("S-001", result->id);
    EXPECT_EQ("반도체시료", result->name);
}

// FR-012: ID 검색 — 존재하지 않는 경우
TEST_F(SampleControllerTest, FindById_NotFound_ReturnsNullopt) {
    EXPECT_CALL(mockRepo_, findById("S-999")).WillOnce(Return(std::nullopt));
    auto result = controller_.findById("S-999");
    EXPECT_FALSE(result.has_value());
}

// FR-032: stock >= CONFIRMED+PRODUCING 수량 합계 → SURPLUS
TEST_F(SampleControllerTest, GetStockStatus_Surplus_WhenStockGteActiveQty) {
    Sample s{"S-001", "시료", 1.0, 0.9, 10};
    EXPECT_CALL(mockRepo_, findAll()).WillOnce(Return(std::vector<Sample>{s}));
    Order o1{"O-001","S-001","고객A",5,OrderStatus::CONFIRMED,"","",0,0};
    Order o2{"O-002","S-001","고객B",3,OrderStatus::PRODUCING,"","",0,0};
    const auto result = controller_.getStockStatus({o1, o2});
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(StockStatus::SURPLUS, result[0].stockStatus);
}

// FR-032: 0 < stock < CONFIRMED+PRODUCING 수량 합계 → SHORTAGE
TEST_F(SampleControllerTest, GetStockStatus_Shortage_WhenStockBetweenZeroAndActiveQty) {
    Sample s{"S-001", "시료", 1.0, 0.9, 3};
    EXPECT_CALL(mockRepo_, findAll()).WillOnce(Return(std::vector<Sample>{s}));
    Order o1{"O-001","S-001","고객A",5,OrderStatus::CONFIRMED,"","",0,0};
    Order o2{"O-002","S-001","고객B",3,OrderStatus::PRODUCING,"","",0,0};
    const auto result = controller_.getStockStatus({o1, o2});
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(StockStatus::SHORTAGE, result[0].stockStatus);
}

// FR-032: stock == 0 → DEPLETED
TEST_F(SampleControllerTest, GetStockStatus_Depleted_WhenStockIsZero) {
    Sample s{"S-001", "시료", 1.0, 0.9, 0};
    EXPECT_CALL(mockRepo_, findAll()).WillOnce(Return(std::vector<Sample>{s}));
    const auto result = controller_.getStockStatus({});
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(StockStatus::DEPLETED, result[0].stockStatus);
}

// FR-032: 활성 주문 없을 때 재고 있으면 → SURPLUS
TEST_F(SampleControllerTest, GetStockStatus_Surplus_WhenNoActiveOrders) {
    Sample s{"S-001", "시료", 1.0, 0.9, 5};
    EXPECT_CALL(mockRepo_, findAll()).WillOnce(Return(std::vector<Sample>{s}));
    const auto result = controller_.getStockStatus({});
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ(StockStatus::SURPLUS, result[0].stockStatus);
}
