#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "MockSampleRepository.h"
#include "MockOrderRepository.h"
#include "DummyDataGenerator.h"
#include "controller/OrderController.h"
#include <algorithm>

using ::testing::Return;
using ::testing::Field;
using ::testing::_;

namespace {
    Order makeOrderWithStatus(const std::string& id, OrderStatus status) {
        return DummyDataGenerator::makeOrder(id, "S-001", "테스트고객", 5, status);
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
    EXPECT_CALL(mockOrderRepo_, findAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::status, OrderStatus::RESERVED))).Times(1);
    EXPECT_TRUE(controller_.createOrder("S-001", "고객A", 5));
}

// FR-020: 기존 주문 없을 때 첫 번째 순번 ID = "O-001"
TEST_F(OrderControllerTest, CreateOrder_NoExistingOrders_IdIsO001) {
    Sample s{"S-001", "시료", 1.0, 0.9, 10};
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{s}));
    EXPECT_CALL(mockOrderRepo_, findAll()).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::id, std::string("O-001")))).Times(1);
    EXPECT_TRUE(controller_.createOrder("S-001", "고객A", 5));
}

// FR-020: 기존 최대 순번이 O-003이면 다음 ID = "O-004"
TEST_F(OrderControllerTest, CreateOrder_ExistingMaxO003_IdIsO004) {
    Sample s{"S-001", "시료", 1.0, 0.9, 10};
    std::vector<Order> existing = {
        DummyDataGenerator::makeOrder("O-003", "S-001", "테스트고객", 2),
        DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 1)
    };
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{s}));
    EXPECT_CALL(mockOrderRepo_, findAll()).WillOnce(Return(existing));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::id, std::string("O-004")))).Times(1);
    EXPECT_TRUE(controller_.createOrder("S-001", "고객A", 5));
}

// FR-021: RESERVED 주문 목록 반환
TEST_F(OrderControllerTest, GetPendingOrders_DelegatesToFindByStatus) {
    std::vector<Order> reserved = { DummyDataGenerator::makeOrder("O-001") };
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
    Order  order  = DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 5);
    Sample sample{"S-001", "시료", 1.0, 0.9, 10};
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::status, OrderStatus::CONFIRMED))).Times(1);
    EXPECT_TRUE(controller_.approveOrder("O-001"));
}

// FR-022: 가용 재고 < quantity → PRODUCING
TEST_F(OrderControllerTest, ApproveOrder_InsufficientStock_BecomesProducing) {
    Order  order  = DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 10);
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
    Order  order  = DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 10);
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
    Order  order    = DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 3);
    Sample sample{"S-001", "시료", 1.0, 0.9, 0};
    Order  existing = DummyDataGenerator::makeOrder("O-000", "S-001", "고객B", 5, OrderStatus::CONFIRMED);
    EXPECT_CALL(mockOrderRepo_,  findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001")).WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_,  findBySampleId("S-001")).WillOnce(Return(std::vector<Order>{existing}));
    EXPECT_CALL(mockOrderRepo_,  save(Field(&Order::targetProductionQuantity, 4))).Times(1);
    controller_.approveOrder("O-001");
}

// FR-023: 재고 0, 다른 주문 없음 → 전체 주문 수량이 부족분
// stock=0, qty=5, yield=0.9 → shortage=5 → ceil(5/0.81)=7
TEST_F(OrderControllerTest, CalcProduction_ZeroStock_FullOrderQty) {
    Order  order  = DummyDataGenerator::makeOrder("O-001", "S-001", "테스트고객", 5);
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
    Order order = DummyDataGenerator::makeOrder("O-001");
    EXPECT_CALL(mockOrderRepo_, findById("O-001")).WillOnce(Return(std::optional<Order>{order}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::status, OrderStatus::REJECTED))).Times(1);
    EXPECT_TRUE(controller_.rejectOrder("O-001"));
}

// FR-030: 상태별 주문 건수 집계 — REJECTED 제외 확인
TEST_F(OrderControllerTest, GetOrderStats_VariousStatuses_CountsExcludeRejected) {
    std::vector<Order> all = {
        makeOrderWithStatus("O-001", OrderStatus::RESERVED),
        makeOrderWithStatus("O-002", OrderStatus::RESERVED),
        makeOrderWithStatus("O-003", OrderStatus::PRODUCING),
        makeOrderWithStatus("O-004", OrderStatus::CONFIRMED),
        makeOrderWithStatus("O-005", OrderStatus::RELEASE),
        makeOrderWithStatus("O-006", OrderStatus::REJECTED),
        makeOrderWithStatus("O-007", OrderStatus::REJECTED)
    };
    EXPECT_CALL(mockOrderRepo_, findAll()).WillOnce(Return(all));
    const OrderStats stats = controller_.getOrderStats();
    EXPECT_EQ(2, stats.reserved);
    EXPECT_EQ(1, stats.producing);
    EXPECT_EQ(1, stats.confirmed);
    EXPECT_EQ(1, stats.release);
}

// FR-030: 주문 없을 때 모든 카운트 0
TEST_F(OrderControllerTest, GetOrderStats_NoOrders_AllZero) {
    EXPECT_CALL(mockOrderRepo_, findAll()).WillOnce(Return(std::vector<Order>{}));
    const OrderStats stats = controller_.getOrderStats();
    EXPECT_EQ(0, stats.reserved);
    EXPECT_EQ(0, stats.producing);
    EXPECT_EQ(0, stats.confirmed);
    EXPECT_EQ(0, stats.release);
}

// FR-030: CONFIRMED + PRODUCING 주문만 반환 (재고 상태 계산용)
TEST_F(OrderControllerTest, GetActiveOrders_ReturnsOnlyConfirmedAndProducing) {
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::CONFIRMED))
        .WillOnce(Return(std::vector<Order>{makeOrderWithStatus("O-002", OrderStatus::CONFIRMED)}));
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{makeOrderWithStatus("O-003", OrderStatus::PRODUCING)}));
    const auto result = controller_.getActiveOrders();
    ASSERT_EQ(2u, result.size());
    EXPECT_TRUE(std::any_of(result.begin(), result.end(),
        [](const Order& o){ return o.status == OrderStatus::CONFIRMED; }));
    EXPECT_TRUE(std::any_of(result.begin(), result.end(),
        [](const Order& o){ return o.status == OrderStatus::PRODUCING; }));
}

// FR-042: 경과 시간 < 1단위 → delta=0, save 없음
TEST_F(OrderControllerTest, UpdateProduction_ElapsedLessThanOneUnit_NoDeltaNoSave) {
    Order order = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 0, 10);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 0);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_, save(_)).Times(0);
    EXPECT_CALL(mockSampleRepo_, save(_)).Times(0);
    controller_.updateProduction("2026-01-01 10:54:00");  // 0.9h 경과
}

// FR-042: 경과 2.5h, avgTime=1.0h → delta=2, stock+=2, producedQty=2
TEST_F(OrderControllerTest, UpdateProduction_Elapsed2_5h_DeltaApplied) {
    Order order = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 0, 10);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 5);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{sample}));
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::producedQuantity, 2))).Times(1);
    EXPECT_CALL(mockSampleRepo_, save(Field(&Sample::stock, 7))).Times(1);  // 5+2
    controller_.updateProduction("2026-01-01 12:30:00");  // 2.5h 경과
}

// FR-042: floor(elapsed/avgTime) > targetQty → producedQty 클램프
TEST_F(OrderControllerTest, UpdateProduction_DeltaExceedsRemaining_ClampedToTarget) {
    Order order = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 8, 10);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 3);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{sample}));
    // floor(12/1)=12 → capped to 10 → delta=2 (not 4)
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::producedQuantity, 10))).Times(1);
    EXPECT_CALL(mockSampleRepo_, save(Field(&Sample::stock, 5))).Times(1);  // 3+2
    controller_.updateProduction("2026-01-01 22:00:00");  // 12h 경과
}

// FR-042: 총 생산시간 경과 → PRODUCING → CONFIRMED 전환
TEST_F(OrderControllerTest, UpdateProduction_TotalTimeElapsed_BecomesConfirmed) {
    Order order = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 0, 10);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 0);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{sample}));
    // 총 생산시간=10h 경과 → producedQty==targetQty → CONFIRMED
    EXPECT_CALL(mockOrderRepo_, save(Field(&Order::status, OrderStatus::CONFIRMED))).Times(1);
    EXPECT_CALL(mockSampleRepo_, save(_)).Times(1);
    controller_.updateProduction("2026-01-01 20:00:00");  // 10h 경과
}

// FR-040: PRODUCING 주문 있음 → ProductionInfo 반환
TEST_F(OrderControllerTest, GetCurrentProduction_HasProducing_ReturnsProductionInfo) {
    Order order = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 0, 10);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 0);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{order}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillOnce(Return(std::optional<Sample>{sample}));
    const auto result = controller_.getCurrentProduction();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("O-001", result->order.id);
    EXPECT_DOUBLE_EQ(10.0, result->totalProductionHours);  // 1.0 × 10
}

// FR-040: PRODUCING 주문 없음 → nullopt 반환
TEST_F(OrderControllerTest, GetCurrentProduction_NoProducing_ReturnsNullopt) {
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{}));
    const auto result = controller_.getCurrentProduction();
    EXPECT_FALSE(result.has_value());
}

// FR-041: PRODUCING 주문 여러 개 → 전체 반환
TEST_F(OrderControllerTest, GetProductionQueue_MultipleProducing_ReturnsAll) {
    Order o1 = DummyDataGenerator::makeOrder(
        "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 10:00:00", 0, 10);
    Order o2 = DummyDataGenerator::makeOrder(
        "O-002", "S-001", "고객B", 3, OrderStatus::PRODUCING,
        "2026-01-01 10:00:00", "2026-01-01 11:00:00", 0, 8);
    Sample sample = DummyDataGenerator::makeSample("S-001", "시료", 1.0, 0.9, 0);
    EXPECT_CALL(mockOrderRepo_, findByStatus(OrderStatus::PRODUCING))
        .WillOnce(Return(std::vector<Order>{o1, o2}));
    EXPECT_CALL(mockSampleRepo_, findById("S-001"))
        .WillRepeatedly(Return(std::optional<Sample>{sample}));
    const auto result = controller_.getProductionQueue();
    EXPECT_EQ(2u, result.size());
}
