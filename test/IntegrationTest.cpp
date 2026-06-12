#include "RepositoryTestFixture.h"
#include "repository/SampleRepository.h"
#include "repository/OrderRepository.h"
#include "controller/SampleController.h"
#include "controller/OrderController.h"

// ---------------------------------------------------------------------------
// IT-01 계열: 시료 Repository 왕복 (미작성 항목 구현)
// ---------------------------------------------------------------------------

class SampleRepositoryIT : public RepositoryTestFixture {};

// IT-01f: 이름 부분 일치 검색 왕복
TEST_F(SampleRepositoryIT, IT01f_FindByName_PartialMatch_ReturnsTwoItems) {
    SampleRepository repo{ testDataDir_ + "/samples.json" };
    repo.save(Sample{ "S-001", "반도체시료A", 1.0, 0.9, 0 });
    repo.save(Sample{ "S-002", "반도체시료B", 1.0, 0.9, 0 });
    repo.save(Sample{ "S-003", "다른시료",   1.0, 0.9, 0 });

    SampleRepository repo2{ testDataDir_ + "/samples.json" };
    auto results = repo2.findByName("반도체");
    ASSERT_EQ(2u, results.size());
    // 미일치 항목은 포함되지 않음
    for (const auto& s : results) {
        EXPECT_NE("S-003", s.id);
    }
}

// IT-01g: 이름 검색 미일치
TEST_F(SampleRepositoryIT, IT01g_FindByName_NoMatch_ReturnsEmpty) {
    SampleRepository repo{ testDataDir_ + "/samples.json" };
    repo.save(Sample{ "S-001", "다른시료", 1.0, 0.9, 0 });

    SampleRepository repo2{ testDataDir_ + "/samples.json" };
    auto results = repo2.findByName("없는이름");
    EXPECT_TRUE(results.empty());
}

// IT-01h: 재기동 후 데이터 유지
TEST_F(SampleRepositoryIT, IT01h_Persistence_ReloadAfterSave_RetainsData) {
    {
        SampleRepository repo{ testDataDir_ + "/samples.json" };
        repo.save(Sample{ "S-001", "산화막시료", 2.0, 0.85, 50 });
    }
    // 새 인스턴스로 재로드
    SampleRepository repo2{ testDataDir_ + "/samples.json" };
    auto found = repo2.findById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(0.85, found->yield);
    EXPECT_EQ(50, found->stock);
}

// ---------------------------------------------------------------------------
// IT-02 계열: 주문 Repository 왕복 (미작성 항목 구현)
// ---------------------------------------------------------------------------

class OrderRepositoryIT : public RepositoryTestFixture {};

// IT-02g: findByStatus — 상태별 필터 왕복
TEST_F(OrderRepositoryIT, IT02g_FindByStatus_ReturnsFilteredOrders) {
    OrderRepository repo{ testDataDir_ + "/orders.json" };
    repo.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });
    repo.save(Order{ "O-002", "S-001", "고객B", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });
    repo.save(Order{ "O-003", "S-001", "고객C", 5, OrderStatus::CONFIRMED,
                     "2026-06-12 00:00:00", "", 0, 0 });

    OrderRepository repo2{ testDataDir_ + "/orders.json" };
    auto results = repo2.findByStatus(OrderStatus::RESERVED);
    ASSERT_EQ(2u, results.size());
    for (const auto& o : results) {
        EXPECT_EQ(OrderStatus::RESERVED, o.status);
    }
}

// IT-02h: findBySampleId — 시료 ID 필터 왕복
TEST_F(OrderRepositoryIT, IT02h_FindBySampleId_ReturnsTwoOrders) {
    OrderRepository repo{ testDataDir_ + "/orders.json" };
    repo.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });
    repo.save(Order{ "O-002", "S-001", "고객B", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });
    repo.save(Order{ "O-003", "S-002", "고객C", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });

    OrderRepository repo2{ testDataDir_ + "/orders.json" };
    auto results = repo2.findBySampleId("S-001");
    ASSERT_EQ(2u, results.size());
    for (const auto& o : results) {
        EXPECT_EQ("S-001", o.sampleId);
    }
}

// IT-02i: findBySampleId — 미일치
TEST_F(OrderRepositoryIT, IT02i_FindBySampleId_NoMatch_ReturnsEmpty) {
    OrderRepository repo{ testDataDir_ + "/orders.json" };
    repo.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::RESERVED,
                     "2026-06-12 00:00:00", "", 0, 0 });

    OrderRepository repo2{ testDataDir_ + "/orders.json" };
    auto results = repo2.findBySampleId("S-999");
    EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// IT-03~14 계열: 비즈니스 흐름 (Controller + 실제 Repository 연동)
// ---------------------------------------------------------------------------

class BusinessFlowIT : public RepositoryTestFixture {
protected:
    SampleRepository sampleRepo_{ testDataDir_ + "/samples.json" };
    OrderRepository  orderRepo_{  testDataDir_ + "/orders.json"  };
    SampleController sampleCtrl_{ sampleRepo_ };
    OrderController  orderCtrl_{  sampleRepo_, orderRepo_ };
};

// IT-03: 주문 승인 — 재고 충분 → CONFIRMED 저장
TEST_F(BusinessFlowIT, IT03_ApproveOrder_SufficientStock_SavesConfirmed) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 2.0, 0.9, 10 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));

    // 새 인스턴스로 재로드하여 영속성 확인
    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto found = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(OrderStatus::CONFIRMED, found->status);

    // 재고는 즉시 차감되지 않음
    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sample = reloadedSampleRepo.findById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(10, sample->stock);
}

// IT-04: 주문 승인 — 재고 부족 → PRODUCING + 생산량 계산 저장
// 사전 조건: S-001(stock=3, yield=0.9, avgTime=2.0h), O-001(qty=10, RESERVED)
// 부족분 = 10 - 3 = 7, targetProductionQuantity = ceil(7 / (0.9 * 0.9)) = ceil(7/0.81) = 9
TEST_F(BusinessFlowIT, IT04_ApproveOrder_InsufficientStock_SavesProducingWithCalcQty) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 2.0, 0.9, 3 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 10, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto found = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(OrderStatus::PRODUCING, found->status);
    EXPECT_EQ(9, found->targetProductionQuantity);
    EXPECT_FALSE(found->productionStartedAt.empty());

    // 재고 즉시 차감 없음
    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sample = reloadedSampleRepo.findById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(3, sample->stock);
}

// IT-05: 생산 진행 시뮬레이션 — producedQuantity 증가 반영
// avgTime=1.0h, target=5, startedAt="2026-06-12 08:00:00"
// updateProduction("2026-06-12 10:30:00") → elapsed=2.5h, ceil(2.5/1.0)=3, delta=3
TEST_F(BusinessFlowIT, IT05_UpdateProduction_PartialProgress_IncreasesProducedQty) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 0 });
    Order o{ "O-001", "S-001", "고객A", 5, OrderStatus::PRODUCING,
             "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 5 };
    orderRepo_.save(o);

    orderCtrl_.updateProduction("2026-06-12 10:30:00");

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto found = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(3, found->producedQuantity);
    EXPECT_EQ(OrderStatus::PRODUCING, found->status);  // target=5 미도달

    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sample = reloadedSampleRepo.findById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(3, sample->stock);  // stock = 0 + 3
}

// IT-06: 생산 완료 시뮬레이션 — PRODUCING → CONFIRMED 전환 저장
// avgTime=2.0h, target=3, startedAt="2026-06-12 08:00:00"
// updateProduction("2026-06-12 14:00:01") → elapsed≈6h, ceil(6/2.0)=3 → 완료 → CONFIRMED
TEST_F(BusinessFlowIT, IT06_UpdateProduction_Completed_TransitionsToConfirmed) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 2.0, 0.9, 0 });
    Order o{ "O-001", "S-001", "고객A", 3, OrderStatus::PRODUCING,
             "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 3 };
    orderRepo_.save(o);

    orderCtrl_.updateProduction("2026-06-12 14:00:01");

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto found = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(OrderStatus::CONFIRMED, found->status);
    EXPECT_EQ(3, found->producedQuantity);
}

// IT-07: 출고 처리 — CONFIRMED → RELEASE + stock 감소 저장
TEST_F(BusinessFlowIT, IT07_ReleaseOrder_Confirmed_ReleasesAndReducesStock) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 2.0, 0.9, 10 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    ASSERT_TRUE(orderCtrl_.releaseOrder("O-001"));

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto found = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(OrderStatus::RELEASE, found->status);

    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sample = reloadedSampleRepo.findById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(5, sample->stock);  // 10 - 5 = 5
}

// IT-08: 모니터링 — 다수 주문 상태별 집계 (JSON 파일 기반)
// RESERVED×2, PRODUCING×1, CONFIRMED×1, RELEASE×1, REJECTED×2
TEST_F(BusinessFlowIT, IT08_GetOrderStats_MultipleStatuses_CorrectAggregation) {
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 5, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-003", "S-001", "고객C", 5, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 5 });
    orderRepo_.save(Order{ "O-004", "S-001", "고객D", 5, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-005", "S-001", "고객E", 5, OrderStatus::RELEASE,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-006", "S-001", "고객F", 5, OrderStatus::REJECTED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-007", "S-001", "고객G", 5, OrderStatus::REJECTED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    auto stats = orderCtrl_.getOrderStats();
    EXPECT_EQ(2, stats.reserved);
    EXPECT_EQ(1, stats.producing);
    EXPECT_EQ(1, stats.confirmed);
    EXPECT_EQ(1, stats.release);
    // REJECTED는 집계에서 제외
}

// IT-09: 재고 상태 판정 — 실 JSON 기반 SURPLUS / SHORTAGE / DEPLETED 확인
// IT-09a: stock=10, 활성 qty 합계=8(CONFIRMED 5 + PRODUCING 3) → SURPLUS
TEST_F(BusinessFlowIT, IT09a_GetStockStatus_Surplus) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 10 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 3, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 3 });

    auto activeQty = orderCtrl_.getActiveQtyBySample();
    auto statuses = sampleCtrl_.getStockStatus(activeQty);
    ASSERT_EQ(1u, statuses.size());
    EXPECT_EQ(StockStatus::SURPLUS, statuses[0].stockStatus);
}

// IT-09b: stock=3, 활성 qty 합계=8 → SHORTAGE
TEST_F(BusinessFlowIT, IT09b_GetStockStatus_Shortage) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 3 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 5, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 3, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 3 });

    auto activeQty = orderCtrl_.getActiveQtyBySample();
    auto statuses = sampleCtrl_.getStockStatus(activeQty);
    ASSERT_EQ(1u, statuses.size());
    EXPECT_EQ(StockStatus::SHORTAGE, statuses[0].stockStatus);
}

// IT-09c: stock=0 → DEPLETED
TEST_F(BusinessFlowIT, IT09c_GetStockStatus_Depleted) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 0 });

    auto activeQty = orderCtrl_.getActiveQtyBySample();
    auto statuses = sampleCtrl_.getStockStatus(activeQty);
    ASSERT_EQ(1u, statuses.size());
    EXPECT_EQ(StockStatus::DEPLETED, statuses[0].stockStatus);
}

// IT-10: 동일 시료 2개 주문 — 첫 번째 CONFIRMED 후 두 번째 승인 시 가용 재고 차감 검증
// S-001(stock=10, yield=0.9), O-001(qty=6) → CONFIRMED
// O-002(qty=6) → 가용 재고 = 10 - 6 = 4 < 6 → PRODUCING
// targetProductionQuantity = ceil((6-4) / (0.9*0.9)) = ceil(2/0.81) = 3
TEST_F(BusinessFlowIT, IT10_TwoOrders_FirstConfirmed_SecondReducesAvailableStock) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 10 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 6, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 6, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    // 첫 번째 주문 승인
    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));

    {
        OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
        auto o1 = reloadedOrderRepo.findById("O-001");
        ASSERT_TRUE(o1.has_value());
        EXPECT_EQ(OrderStatus::CONFIRMED, o1->status);

        SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
        auto s = reloadedSampleRepo.findById("S-001");
        ASSERT_TRUE(s.has_value());
        EXPECT_EQ(10, s->stock);  // 즉시 차감 없음
    }

    // 두 번째 주문 승인
    ASSERT_TRUE(orderCtrl_.approveOrder("O-002"));

    {
        OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
        auto o2 = reloadedOrderRepo.findById("O-002");
        ASSERT_TRUE(o2.has_value());
        EXPECT_EQ(OrderStatus::PRODUCING, o2->status);
        EXPECT_EQ(3, o2->targetProductionQuantity);  // ceil(2/0.81)=3
    }
}

// IT-11: 동일 시료 2개 주문 — 둘 다 재고 부족 → PRODUCING, targetProductionQuantity 독립 계산
// S-001(stock=2, yield=0.9, avgTime=1.0h)
// O-001(qty=8): 가용=2, 부족=6, target=ceil(6/0.81)=8
// O-002(qty=5): 가용=max(0, 2-8)=0, 부족=5, target=ceil(5/0.81)=7
TEST_F(BusinessFlowIT, IT11_TwoOrders_BothInsufficientStock_IndependentTargetQty) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 2 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 8, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 5, OrderStatus::RESERVED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));

    {
        OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
        auto o1 = reloadedOrderRepo.findById("O-001");
        ASSERT_TRUE(o1.has_value());
        EXPECT_EQ(OrderStatus::PRODUCING, o1->status);
        EXPECT_EQ(8, o1->targetProductionQuantity);
        EXPECT_FALSE(o1->productionStartedAt.empty());
    }

    ASSERT_TRUE(orderCtrl_.approveOrder("O-002"));

    {
        OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
        auto o2 = reloadedOrderRepo.findById("O-002");
        ASSERT_TRUE(o2.has_value());
        EXPECT_EQ(OrderStatus::PRODUCING, o2->status);
        EXPECT_EQ(7, o2->targetProductionQuantity);  // ceil(5/0.81)=7

        // O-001의 targetProductionQuantity는 영향 없음
        auto o1 = reloadedOrderRepo.findById("O-001");
        ASSERT_TRUE(o1.has_value());
        EXPECT_EQ(8, o1->targetProductionQuantity);
    }
}

// IT-12: PRODUCING 2개 — FIFO 단일 생산라인, front만 처리 queue는 대기
// O-001(front, startedAt=08:00): elapsed=3.0h → ceil(3/1)=3, delta=3
// O-002(queue, startedAt=09:00): 처리 대상 아님 → producedQuantity 변화 없음
// stock = 0 + 3 = 3
TEST_F(BusinessFlowIT, IT12_UpdateProduction_TwoProducingOrders_OnlyFrontProcessed) {
    sampleRepo_.save(Sample{ "S-001", "산화막시료", 1.0, 0.9, 0 });
    orderRepo_.save(Order{ "O-001", "S-001", "고객A", 4, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 4 });
    orderRepo_.save(Order{ "O-002", "S-001", "고객B", 6, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 09:00:00", 0, 7 });

    orderCtrl_.updateProduction("2026-06-12 11:00:00");

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto o1 = reloadedOrderRepo.findById("O-001");
    ASSERT_TRUE(o1.has_value());
    EXPECT_EQ(3, o1->producedQuantity);
    EXPECT_EQ(OrderStatus::PRODUCING, o1->status);  // target=4 미도달

    auto o2 = reloadedOrderRepo.findById("O-002");
    ASSERT_TRUE(o2.has_value());
    EXPECT_EQ(0, o2->producedQuantity);             // front 아님, 미처리
    EXPECT_EQ(OrderStatus::PRODUCING, o2->status);

    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sample = reloadedSampleRepo.findById("S-001");
    ASSERT_TRUE(sample.has_value());
    EXPECT_EQ(3, sample->stock);  // 0 + 3 (O-002 미처리)
}

// IT-13: 시료 A·B 각각 주문 — getStockStatus 조회 시 각 시료 상태 독립 계산
// S-A(stock=10): CONFIRMED(7) + PRODUCING(2) = 9 → SURPLUS (10 >= 9)
// S-B(stock=3):  CONFIRMED(8) = 8 → SHORTAGE (3 < 8)
TEST_F(BusinessFlowIT, IT13_GetStockStatus_TwoSamples_IndependentStatus) {
    sampleRepo_.save(Sample{ "S-A", "시료A", 1.0, 0.9, 10 });
    sampleRepo_.save(Sample{ "S-B", "시료B", 1.0, 0.9, 3 });

    orderRepo_.save(Order{ "O-A1", "S-A", "고객A1", 7, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-A2", "S-A", "고객A2", 2, OrderStatus::PRODUCING,
                           "2026-06-12 00:00:00", "2026-06-12 08:00:00", 0, 2 });
    orderRepo_.save(Order{ "O-B1", "S-B", "고객B1", 8, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    auto activeQty = orderCtrl_.getActiveQtyBySample();
    auto statuses = sampleCtrl_.getStockStatus(activeQty);

    ASSERT_EQ(2u, statuses.size());

    StockStatus statusA = StockStatus::SURPLUS;
    StockStatus statusB = StockStatus::SURPLUS;
    for (const auto& info : statuses) {
        if (info.sample.id == "S-A") statusA = info.stockStatus;
        if (info.sample.id == "S-B") statusB = info.stockStatus;
    }
    EXPECT_EQ(StockStatus::SURPLUS,  statusA);
    EXPECT_EQ(StockStatus::SHORTAGE, statusB);
}

// IT-14: 시료 A 주문 출고 — 시료 A stock만 감소, 시료 B stock 영향 없음
TEST_F(BusinessFlowIT, IT14_ReleaseOrder_OnlyTargetSampleStockDecreases) {
    sampleRepo_.save(Sample{ "S-A", "시료A", 1.0, 0.9, 10 });
    sampleRepo_.save(Sample{ "S-B", "시료B", 1.0, 0.9, 7 });
    orderRepo_.save(Order{ "O-A", "S-A", "고객A", 4, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });
    orderRepo_.save(Order{ "O-B", "S-B", "고객B", 3, OrderStatus::CONFIRMED,
                           "2026-06-12 00:00:00", "", 0, 0 });

    ASSERT_TRUE(orderCtrl_.releaseOrder("O-A"));

    OrderRepository reloadedOrderRepo{ testDataDir_ + "/orders.json" };
    auto oA = reloadedOrderRepo.findById("O-A");
    ASSERT_TRUE(oA.has_value());
    EXPECT_EQ(OrderStatus::RELEASE, oA->status);

    auto oB = reloadedOrderRepo.findById("O-B");
    ASSERT_TRUE(oB.has_value());
    EXPECT_EQ(OrderStatus::CONFIRMED, oB->status);  // 변동 없음

    SampleRepository reloadedSampleRepo{ testDataDir_ + "/samples.json" };
    auto sA = reloadedSampleRepo.findById("S-A");
    ASSERT_TRUE(sA.has_value());
    EXPECT_EQ(6, sA->stock);  // 10 - 4 = 6

    auto sB = reloadedSampleRepo.findById("S-B");
    ASSERT_TRUE(sB.has_value());
    EXPECT_EQ(7, sB->stock);  // 변동 없음
}

// ---------------------------------------------------------------------------
// E2E-01: 재고 충분 경로 (RESERVED → CONFIRMED → RELEASE)
// ---------------------------------------------------------------------------

class E2EFlowIT : public RepositoryTestFixture {
protected:
    SampleRepository sampleRepo_{ testDataDir_ + "/samples.json" };
    OrderRepository  orderRepo_{  testDataDir_ + "/orders.json"  };
    SampleController sampleCtrl_{ sampleRepo_ };
    OrderController  orderCtrl_{  sampleRepo_, orderRepo_ };
};

TEST_F(E2EFlowIT, E2E01_SufficientStock_FullFlow) {
    // [1] 시료 등록
    ASSERT_TRUE(sampleCtrl_.registerSample("S-001", "산화막시료", 2.0, 0.9));
    {
        SampleRepository r{ testDataDir_ + "/samples.json" };
        auto s = r.findById("S-001");
        ASSERT_TRUE(s.has_value());
        EXPECT_EQ(0, s->stock);  // registerSample은 stock=0으로 등록
    }
    // stock을 20으로 직접 설정
    {
        auto s = sampleRepo_.findById("S-001").value();
        s.stock = 20;
        sampleRepo_.save(s);
    }
    {
        SampleRepository r{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(20, r.findById("S-001")->stock);
    }

    // [2] 주문 접수
    ASSERT_TRUE(orderCtrl_.createOrder("S-001", "고객A", 10));
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        auto orders = r.findAll();
        ASSERT_EQ(1u, orders.size());
        EXPECT_EQ(OrderStatus::RESERVED, orders[0].status);
        EXPECT_EQ("O-001", orders[0].id);
    }

    // [3] 승인 (가용 재고 20 >= qty 10)
    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        EXPECT_EQ(OrderStatus::CONFIRMED, r.findById("O-001")->status);

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(20, sr.findById("S-001")->stock);  // 즉시 차감 없음
    }

    // [4] 출고 처리
    ASSERT_TRUE(orderCtrl_.releaseOrder("O-001"));
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        EXPECT_EQ(OrderStatus::RELEASE, r.findById("O-001")->status);

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(10, sr.findById("S-001")->stock);  // 20 - 10 = 10
    }
}

// ---------------------------------------------------------------------------
// E2E-02: 재고 부족 경로 (RESERVED → PRODUCING → CONFIRMED → RELEASE)
// ---------------------------------------------------------------------------

TEST_F(E2EFlowIT, E2E02_InsufficientStock_FullFlow) {
    // [1] 시료 등록 (stock=3)
    ASSERT_TRUE(sampleCtrl_.registerSample("S-001", "산화막시료", 1.0, 0.9));
    {
        auto s = sampleRepo_.findById("S-001").value();
        s.stock = 3;
        sampleRepo_.save(s);
    }

    // [2] 주문 접수
    ASSERT_TRUE(orderCtrl_.createOrder("S-001", "고객B", 10));
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        EXPECT_EQ(OrderStatus::RESERVED, r.findById("O-001")->status);
    }

    // [3] 승인 (가용 재고 3 < qty 10)
    ASSERT_TRUE(orderCtrl_.approveOrder("O-001"));

    std::string productionStartedAt;
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        auto found = r.findById("O-001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(OrderStatus::PRODUCING, found->status);
        EXPECT_EQ(9, found->targetProductionQuantity);  // ceil(7/0.81)=9
        EXPECT_FALSE(found->productionStartedAt.empty());
        productionStartedAt = found->productionStartedAt;

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(3, sr.findById("S-001")->stock);  // 즉시 차감 없음
    }

    // [4a] 생산 진행 (T + 4.0h): ceil(4.0/1.0)=4, delta=4
    // productionStartedAt에서 4시간 후 타임스탬프 구성
    // 테스트 단순화: startedAt을 고정 문자열로 덮어쓰기
    {
        auto order = orderRepo_.findById("O-001").value();
        order.productionStartedAt = "2026-06-12 08:00:00";
        orderRepo_.save(order);
    }
    orderCtrl_.updateProduction("2026-06-12 12:00:00");  // T+4h
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        auto found = r.findById("O-001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(4, found->producedQuantity);
        EXPECT_EQ(OrderStatus::PRODUCING, found->status);

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(7, sr.findById("S-001")->stock);  // 3 + 4 = 7
    }

    // [4b] 생산 완료 (T + 9.0h): 총 생산시간 9h 경과, target=9 → CONFIRMED
    orderCtrl_.updateProduction("2026-06-12 17:00:00");  // T+9h
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        auto found = r.findById("O-001");
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(OrderStatus::CONFIRMED, found->status);
        EXPECT_EQ(9, found->producedQuantity);

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(12, sr.findById("S-001")->stock);  // 3 + 9 = 12
    }

    // [5] 출고 처리
    ASSERT_TRUE(orderCtrl_.releaseOrder("O-001"));
    {
        OrderRepository r{ testDataDir_ + "/orders.json" };
        EXPECT_EQ(OrderStatus::RELEASE, r.findById("O-001")->status);

        SampleRepository sr{ testDataDir_ + "/samples.json" };
        EXPECT_EQ(2, sr.findById("S-001")->stock);  // 12 - 10 = 2
    }
}
