#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MockSampleRepository.h"
#include "MockOrderRepository.h"
#include "controller/OrderController.h"

using ::testing::Return;
using ::testing::Field;
using ::testing::_;

namespace {
    Order makeReservedOrder(const std::string& id, const std::string& sampleId, int qty) {
        return Order{id, sampleId, "고객A", qty, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0};
    }
}

class OrderControllerTest : public ::testing::Test {
protected:
    MockSampleRepository mockSampleRepo_;
    MockOrderRepository  mockOrderRepo_;
    OrderController      controller_{ mockSampleRepo_, mockOrderRepo_ };
};

// FR-020: 미등록 시료 ID로 주문 시 거부
TEST_F(OrderControllerTest, CreateOrder_UnknownSampleId_ReturnsFalse) {
    EXPECT_CALL(mockSampleRepo_, findById("S-999")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(mockOrderRepo_, save(_)).Times(0);
    EXPECT_FALSE(controller_.createOrder("S-999", "고객A", 5));
}

// FR-020: 정상 주문 생성 → 상태 RESERVED, save 1회 호출
TEST_F(OrderControllerTest, CreateOrder_ValidSample_StatusReserved) {
    Sample s{"S-001", "시료", 1.0, 0.9, 10};
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{s}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::status, OrderStatus::RESERVED))).Times(1);
    EXPECT_TRUE(controller_.createOrder("S-001", "고객A", 5));
}

// FR-021: RESERVED 주문 목록 반환
TEST_F(OrderControllerTest, GetPendingOrders_DelegatesToFindByStatus) {
    std::vector<Order> reserved = { makeReservedOrder("O-001", "S-001", 5) };
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::RESERVED)).WillOnce(Return(reserved));
    auto result = controller_.getPendingOrders();
    ASSERT_EQ(1u, result.size());
    EXPECT_EQ("O-001", result[0].id);
}

// FR-022: 주문 미존재 시 거부
TEST_F(OrderControllerTest, ApproveOrder_NotFound_ReturnsFalse) {
    EXPECT_CALL(mockOrderRepo_, findById("O-999")).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(controller_.approveOrder("O-999"));
}

// FR-022: 가용 재고 >= quantity → CONFIRMED
TEST_F(OrderControllerTest, ApproveOrder_SufficientStock_BecomesConfirmed) {
    Order  order  = makeReservedOrder("O-001", "S-001", 5);
    Sample sample{"S-001", "시료", 1.0, 0.9, 10};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::status, OrderStatus::CONFIRMED))).Times(1);
    EXPECT_TRUE(controller_.approveOrder("O-001"));
}

// FR-022: 가용 재고 < quantity → PRODUCING
TEST_F(OrderControllerTest, ApproveOrder_InsufficientStock_BecomesProducing) {
    Order  order  = makeReservedOrder("O-001", "S-001", 10);
    Sample sample{"S-001", "시료", 1.0, 0.9, 3};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::status, OrderStatus::PRODUCING))).Times(1);
    EXPECT_TRUE(controller_.approveOrder("O-001"));
}

// FR-023: 정상 케이스 — ceil 검증
// stock=3, qty=10, yield=0.9 → available=3, shortage=7 → ceil(7/0.81)=9
TEST_F(OrderControllerTest, CalcProduction_Normal_CeilApplied) {
    Order  order  = makeReservedOrder("O-001", "S-001", 10);
    Sample sample{"S-001", "시료", 1.0, 0.9, 3};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::targetProductionQuantity, 9))).Times(1);
    controller_.approveOrder("O-001");
}

// FR-023: 가용 재고 음수 → 0으로 클립
// stock=0, 기존 CONFIRMED qty=5, 신규 qty=3
// available=-5 → max(0,-5)=0 → shortage=3 → ceil(3/0.81)=4
TEST_F(OrderControllerTest, CalcProduction_NegativeAvailable_UsesZero) {
    Order  order    = makeReservedOrder("O-001", "S-001", 3);
    Sample sample{"S-001", "시료", 1.0, 0.9, 0};
    Order  existing{"O-000", "S-001", "고객B", 5, OrderStatus::CONFIRMED,
                    "2026-06-12 00:00:00", "", 0, 0};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{existing}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::targetProductionQuantity, 4))).Times(1);
    controller_.approveOrder("O-001");
}

// FR-023: 재고 0, 다른 주문 없음 → 전체 주문 수량이 부족분
// stock=0, qty=5, yield=0.9 → shortage=5 → ceil(5/0.81)=7
TEST_F(OrderControllerTest, CalcProduction_ZeroStock_FullOrderQty) {
    Order  order  = makeReservedOrder("O-001", "S-001", 5);
    Sample sample{"S-001", "시료", 1.0, 0.9, 0};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::targetProductionQuantity, 7))).Times(1);
    controller_.approveOrder("O-001");
}

// FR-024: 주문 미존재 시 거절 거부
TEST_F(OrderControllerTest, RejectOrder_NotFound_ReturnsFalse) {
    EXPECT_CALL(mockOrderRepo_, findById("O-999")).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(controller_.rejectOrder("O-999"));
}

// FR-024: RESERVED → REJECTED 전환
TEST_F(OrderControllerTest, RejectOrder_StatusBecomesRejected) {
    Order order = makeReservedOrder("O-001", "S-001", 5);
    EXPECT_CALL(mockOrderRepo_, findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::status, OrderStatus::REJECTED))).Times(1);
    EXPECT_TRUE(controller_.rejectOrder("O-001"));
}
