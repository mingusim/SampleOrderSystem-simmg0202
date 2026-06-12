#include "RepositoryTestFixture.h"
#include "repository/OrderRepository.h"

class OrderRepositoryTest : public RepositoryTestFixture {
protected:
    OrderRepository repo_{ testDataDir_ + "/orders.json" };
};

TEST_F(OrderRepositoryTest, SaveAndFindAll_ReturnsSavedOrder) {
    Order o{ "O-001", "S-001", "테스트고객", 10, OrderStatus::RESERVED,
             "2026-06-12 00:00:00", "", 0 };
    repo_.save(o);
    auto all = repo_.findAll();
    ASSERT_EQ(1u, all.size());
    EXPECT_EQ("O-001", all[0].id);
    EXPECT_EQ(OrderStatus::RESERVED, all[0].status);
}

TEST_F(OrderRepositoryTest, SaveAndFindById_ReturnsCorrectOrder) {
    Order o{ "O-001", "S-001", "테스트고객", 10, OrderStatus::RESERVED,
             "2026-06-12 00:00:00", "", 0 };
    repo_.save(o);
    auto found = repo_.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ("O-001", found->id);
    EXPECT_EQ(10, found->quantity);
}

TEST_F(OrderRepositoryTest, FindById_NotFound_ReturnsNullopt) {
    auto found = repo_.findById("NOTEXIST");
    EXPECT_FALSE(found.has_value());
}

TEST_F(OrderRepositoryTest, Remove_RemovesOrder) {
    Order o{ "O-001", "S-001", "테스트고객", 10, OrderStatus::RESERVED,
             "2026-06-12 00:00:00", "", 0 };
    repo_.save(o);
    repo_.remove("O-001");
    EXPECT_TRUE(repo_.findAll().empty());
}

TEST_F(OrderRepositoryTest, Save_Upsert_UpdatesExistingOrder) {
    Order o{ "O-001", "S-001", "테스트고객", 10, OrderStatus::RESERVED,
             "2026-06-12 00:00:00", "", 0 };
    repo_.save(o);
    o.status = OrderStatus::CONFIRMED;
    repo_.save(o);
    auto all = repo_.findAll();
    ASSERT_EQ(1u, all.size());
    EXPECT_EQ(OrderStatus::CONFIRMED, all[0].status);
}

TEST_F(OrderRepositoryTest, Persistence_ReloadAfterSave_RetainsData) {
    Order o{ "O-001", "S-001", "테스트고객", 10, OrderStatus::PRODUCING,
             "2026-06-12 00:00:00", "2026-06-12 01:00:00", 5 };
    repo_.save(o);
    // 동일 경로로 새 인스턴스 생성 → 파일에서 재로드
    OrderRepository repo2{ testDataDir_ + "/orders.json" };
    auto found = repo2.findById("O-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(OrderStatus::PRODUCING, found->status);
    EXPECT_EQ(5, found->producedQuantity);
    EXPECT_EQ("2026-06-12 01:00:00", found->productionStartedAt);
}
