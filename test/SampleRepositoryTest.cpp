#include "RepositoryTestFixture.h"
#include "repository/SampleRepository.h"

class SampleRepositoryTest : public RepositoryTestFixture {
protected:
    SampleRepository repo_{ testDataDir_ + "/samples.json" };
};

TEST_F(SampleRepositoryTest, SaveAndFindAll_ReturnsSavedSample) {
    Sample s{ "S-001", "테스트시료", 1.0, 0.9, 0 };
    repo_.save(s);
    auto all = repo_.findAll();
    ASSERT_EQ(1u, all.size());
    EXPECT_EQ("S-001", all[0].id);
    EXPECT_EQ("테스트시료", all[0].name);
}

TEST_F(SampleRepositoryTest, SaveAndFindById_ReturnsCorrectSample) {
    Sample s{ "S-001", "테스트시료", 1.0, 0.9, 0 };
    repo_.save(s);
    auto found = repo_.findById("S-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ("S-001", found->id);
    EXPECT_DOUBLE_EQ(0.9, found->yield);
}

TEST_F(SampleRepositoryTest, FindById_NotFound_ReturnsNullopt) {
    auto found = repo_.findById("NOTEXIST");
    EXPECT_FALSE(found.has_value());
}

TEST_F(SampleRepositoryTest, Remove_RemovesSample) {
    Sample s{ "S-001", "테스트시료", 1.0, 0.9, 0 };
    repo_.save(s);
    repo_.remove("S-001");
    EXPECT_TRUE(repo_.findAll().empty());
}

TEST_F(SampleRepositoryTest, Save_Upsert_UpdatesExistingSample) {
    Sample s{ "S-001", "원본", 1.0, 0.9, 0 };
    repo_.save(s);
    s.name = "수정됨";
    s.stock = 100;
    repo_.save(s);
    auto all = repo_.findAll();
    ASSERT_EQ(1u, all.size());
    EXPECT_EQ("수정됨", all[0].name);
    EXPECT_EQ(100, all[0].stock);
}
