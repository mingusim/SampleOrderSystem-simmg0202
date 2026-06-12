# TEST_PLAN.md — 통합 테스트 플랜

> **기준일**: 2026-06-12
> **프레임워크**: Google Mock 1.11.0 (NuGet 패키지)
> **범위**: 통합 테스트 전용 — 단위 테스트는 TDD 사이클에서 별도 관리

---

## 1. 목표

단위 테스트(Controller + Mock)가 비즈니스 로직을 격리 검증하는 것과 달리,
통합 테스트는 **실제 파일 I/O를 포함한 레이어 간 연동**을 검증한다.

| 검증 대상 | 설명 |
|---|---|
| JSON 직렬화·역직렬화 왕복 | 저장 후 재로드 시 타입·정밀도·인코딩 손실 없이 일치하는지 |
| Repository → Controller 연동 | 실제 Repository 주입 시 비즈니스 로직이 파일에 올바르게 반영되는지 |
| 상태 전이 영속성 | 주문 상태 전이가 JSON 파일에 정확히 기록되는지 |
| 생산량 계산 + 파일 반영 | 타임스탬프 주입으로 경과 시간을 제어했을 때 재고·생산량 변경이 반영되는지 |
| 재고 상태 집계 | 실 JSON 데이터 기반으로 SURPLUS / SHORTAGE / DEPLETED 판정이 올바른지 |
| E2E 완전 흐름 | 시료 등록 → 주문 접수 → 승인/생산 → 출고까지 데이터 일관성 유지 여부 |

---

## 2. 테스트 환경 설정

### 2-1. RepositoryTestFixture

모든 통합 테스트는 `RepositoryTestFixture`를 상속하여 파일 격리를 보장한다.

```cpp
// test/RepositoryTestFixture.h
class RepositoryTestFixture : public ::testing::Test {
protected:
    std::string dataDir_;   // "test_data_temp/"

    void SetUp() override {
        dataDir_ = "test_data_temp/";
        std::filesystem::create_directories(dataDir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(dataDir_);
    }
};
```

- `SetUp()`: 임시 디렉터리 `test_data_temp/` 생성
- `TearDown()`: 디렉터리 및 하위 파일 전체 삭제
- 각 테스트는 독립된 파일 공간에서 실행 → 테스트 간 상태 오염 없음

### 2-2. Repository 인스턴스 생성

```cpp
SampleRepository sampleRepo(dataDir_ + "samples.json");
OrderRepository  orderRepo(dataDir_ + "orders.json");

// Controller에 실제 Repository 주입 (Mock 미사용)
SampleController sampleCtrl(sampleRepo);
OrderController  orderCtrl(sampleRepo, orderRepo);
```

### 2-3. 영속성 검증 패턴

JSON 영속성 확인 시 **새 인스턴스를 동일 경로로 다시 생성**하여 검증한다.

```cpp
// 1단계: 저장
SampleRepository repo1(dataDir_ + "samples.json");
repo1.save(sample);

// 2단계: 새 인스턴스로 재로드 후 검증
SampleRepository repo2(dataDir_ + "samples.json");
auto result = repo2.findById("S-001");
ASSERT_TRUE(result.has_value());
EXPECT_EQ(result->name, sample.name);
```

---

## 3. 생산 시간 제어 전략

`OrderController::updateProduction(const std::string& now)` 은 현재 시각을 파라미터로 외부 주입받는다.

- **프로덕션**: View가 `currentTimestamp()` 반환값을 전달
- **테스트**: 고정 타임스탬프 문자열을 직접 전달 → 결정적(deterministic) 검증

### 타임스탬프 형식

```
YYYY-MM-DD HH:MM:SS
예) "2026-06-12 10:00:00"
```

### 경과 시간 계산 예시

```
productionStartedAt = "2026-06-12 08:00:00"
now                 = "2026-06-12 10:30:00"
elapsed             = 2.5시간

avgProductionTime   = 1.0 h/개
ceil(2.5 / 1.0)     = 3  →  총 생산량 = 3
delta               = 3 - producedQuantity(현재값)
```

### 시나리오별 기대 결과

| 시나리오 | startedAt | now | avgProductionTime | 기대 결과 |
|---|---|---|---|---|
| delta = 0 (진행 없음) | 10:00:00 | 10:30:00 | 2.0h | ceil(0.5/2.0)=1, delta=1 |
| delta = 3 (중간 진행) | 08:00:00 | 10:30:00 | 1.0h | ceil(2.5/1.0)=3 |
| target 클램프 | 00:00:00 | 20:00:00 | 1.0h, target=5 | ceil(20/1)=20 → 5로 클램프 |
| 생산 완료 전환 | 08:00:00 | 14:00:01 | 2.0h, target=3 | 총 6h 경과 → CONFIRMED 전환 |

---

## 4. 통합 테스트 목록

### 4-1. 시료 Repository

| ID | 시나리오 | 대상 FR | 검증 포인트 | 상태 |
|---|---|---|---|---|
| IT-01a | 시료 저장 후 findAll 왕복 | FR-010 | 크기 1, id·name·yield·avgProductionTime 일치 | GREEN |
| IT-01b | 시료 저장 후 findById 왕복 | FR-010 | `has_value()` = true, yield 소수점 정밀도 일치 | GREEN |
| IT-01c | 존재하지 않는 ID 조회 | FR-012 | `has_value()` = false | GREEN |
| IT-01d | 시료 삭제 후 목록 비어있음 | FR-010 | 빈 목록 반환 | GREEN |
| IT-01e | 동일 ID 재저장 시 덮어쓰기 | FR-010 | 크기 1, stock = 10 | GREEN |
| IT-01f | 이름 부분 일치 검색 왕복 | FR-012 | 2건 반환, 미일치 항목 미포함 | 미작성 |
| IT-01g | 이름 검색 미일치 | FR-012 | 빈 목록 반환 | 미작성 |
| IT-01h | 재기동 후 데이터 유지 | NFR: 영속성 | yield·stock 값 일치 | 미작성 |

**사전 조건 상세**

- IT-01f: "반도체시료A", "반도체시료B", "다른시료" 저장 후 `findByName("반도체")` 호출
- IT-01g: "다른시료" 저장 후 `findByName("없는이름")` 호출
- IT-01h: `save` 완료 후 동일 경로로 새 인스턴스 생성하여 `findById` 호출

---

### 4-2. 주문 Repository

| ID | 시나리오 | 대상 FR | 검증 포인트 | 상태 |
|---|---|---|---|---|
| IT-02a | 주문 저장 후 findAll 왕복 | FR-020 | 크기 1, id·status·quantity 일치 | GREEN |
| IT-02b | 주문 저장 후 findById 왕복 | FR-020 | `has_value()` = true | GREEN |
| IT-02c | 존재하지 않는 주문 조회 | — | `has_value()` = false | GREEN |
| IT-02d | 주문 삭제 확인 | — | 빈 목록 반환 | GREEN |
| IT-02e | 상태 업데이트 영속성 | FR-022 | 크기 1, status = CONFIRMED | GREEN |
| IT-02f | PRODUCING 주문 타임스탬프 왕복 | FR-042 | status·producedQuantity·productionStartedAt 모두 일치 | GREEN |
| IT-02g | findByStatus — 상태별 필터 왕복 | FR-021, FR-050 | 2건 반환, 다른 상태 미포함 | 미작성 |
| IT-02h | findBySampleId — 시료 ID 필터 왕복 | FR-020 | 2건 반환 | 미작성 |
| IT-02i | findBySampleId — 미일치 | — | 빈 목록 반환 | 미작성 |

**사전 조건 상세**

- IT-02g: RESERVED 2건, CONFIRMED 1건 저장 후 `findByStatus(RESERVED)` 호출
- IT-02h: S-001 주문 2건, S-002 주문 1건 저장 후 `findBySampleId("S-001")` 호출
- IT-02i: S-001 주문만 저장 후 `findBySampleId("S-999")` 호출

---

### 4-3. 비즈니스 흐름

| ID | 시나리오 | 대상 FR | 상태 |
|---|---|---|---|
| IT-03 | 주문 승인 — 재고 충분 → CONFIRMED 저장 | FR-022 | 미작성 |
| IT-04 | 주문 승인 — 재고 부족 → PRODUCING + 생산량 계산 저장 | FR-022, FR-023 | 미작성 |
| IT-05 | 생산 진행 시뮬레이션 — producedQuantity 증가 반영 | FR-042 | 미작성 |
| IT-06 | 생산 완료 시뮬레이션 — PRODUCING → CONFIRMED 전환 저장 | FR-042 | 미작성 |
| IT-07 | 출고 처리 — CONFIRMED → RELEASE + stock 감소 저장 | FR-051 | 미작성 |
| IT-08 | 모니터링 — 다수 주문 상태별 집계 (JSON 파일 기반) | FR-030 | 미작성 |
| IT-09 | 재고 상태 — 실 JSON 기반 SURPLUS / SHORTAGE / DEPLETED 확인 | FR-031, FR-032 | 미작성 |
| IT-10 | 동일 시료 2개 주문 — 첫 번째 CONFIRMED 후 두 번째 승인 시 가용 재고 차감 검증 | FR-022, FR-031 | 미작성 |
| IT-11 | 동일 시료 2개 주문 — 둘 다 재고 부족 → PRODUCING, targetProductionQuantity 독립 계산 검증 | FR-022, FR-023 | 미작성 |
| IT-12 | 동일 시료 PRODUCING 2개 — updateProduction 호출 시 각 주문 생산량 독립 갱신 검증 | FR-042 | 미작성 |
| IT-13 | 시료 A·B 각각 주문 — getStockStatus 조회 시 각 시료 상태 독립 계산 (activeQtyBySample 집계) 검증 | FR-031, FR-032 | 미작성 |
| IT-14 | 시료 A 주문 출고 — 시료 A stock만 감소, 시료 B stock 영향 없음 검증 | FR-051 | 미작성 |

**각 시나리오 상세**

**IT-03** 재고 충분 승인
- 사전 조건: S-001(stock=10), O-001(qty=5, RESERVED) JSON 저장
- 실행: `OrderController::approveOrder("O-001")`
- 검증: orders.json 재로드 → status = CONFIRMED, stock 변동 없음

**IT-04** 재고 부족 승인
- 사전 조건: S-001(stock=3, yield=0.9, avgTime=2.0h), O-001(qty=10, RESERVED) JSON 저장
- 실행: `OrderController::approveOrder("O-001")`
- 검증: status = PRODUCING, targetProductionQuantity = `ceil(7/0.81) = 9`, productionStartedAt 비어 있지 않음

**IT-05** 생산 진행
- 사전 조건: PRODUCING 주문(avgTime=1.0h, target=5, producedQty=0, startedAt="2026-06-12 08:00:00") 저장
- 실행: `updateProduction("2026-06-12 10:30:00")`
- 검증: producedQuantity = 3, stock += 3  // ceil(2.5/1.0)=3

**IT-06** 생산 완료
- 사전 조건: PRODUCING 주문(avgTime=2.0h, target=3, producedQty=0, startedAt="2026-06-12 08:00:00") 저장
- 실행: `updateProduction("2026-06-12 14:00:01")` (총 생산시간 6h 경과)
- 검증: status = CONFIRMED, producedQuantity = 3

**IT-07** 출고 처리
- 사전 조건: S-001(stock=10), O-001(qty=5, CONFIRMED) JSON 저장
- 실행: `OrderController::releaseOrder("O-001")`
- 검증: status = RELEASE, stock = 5

**IT-08** 모니터링 집계
- 사전 조건: RESERVED×2, PRODUCING×1, CONFIRMED×1, RELEASE×1, REJECTED×2 저장
- 실행: `OrderController::getOrderStats()`
- 검증: reserved=2, producing=1, confirmed=1, release=1 (REJECTED 제외)

**IT-09** 재고 상태 판정
- 실행: 실제 Repository 데이터로 `SampleController::getStockStatus()` 호출

| ID | 케이스 | stock | 활성 주문 qty 합계 | 기대 결과 |
|---|---|---|---|---|
| IT-09a | 여유 | 10 | CONFIRMED(5) + PRODUCING(3) = 8 | SURPLUS |
| IT-09b | 부족 | 3 | CONFIRMED(5) + PRODUCING(3) = 8 | SHORTAGE |
| IT-09c | 고갈 | 0 | (무관) | DEPLETED |

---

**IT-10** 동일 시료 2개 주문 — 첫 번째 CONFIRMED 후 두 번째 승인 시 가용 재고 차감 검증
- 사전 조건: S-001(stock=10), O-001(qty=6, RESERVED), O-002(qty=6, RESERVED) JSON 저장
- 실행 1: `OrderController::approveOrder("O-001")` → O-001 status = CONFIRMED
- 검증 1: orders.json 재로드 → O-001 status = CONFIRMED, S-001 stock = 10 (즉시 차감 없음)
- 실행 2: `OrderController::approveOrder("O-002")`
  - 가용 재고 = stock(10) − CONFIRMED 수량 합계(6) = 4 < qty(6) → 재고 부족
- 검증 2: O-002 status = PRODUCING, `calcReservedQuantity("S-001")` = 6 (O-001 수량 포함)
  - `targetProductionQuantity` = `ceil((6−4) / (0.9 × 0.9))` = `ceil(2/0.81)` = 3

**IT-11** 동일 시료 2개 주문 — 둘 다 재고 부족 → PRODUCING, targetProductionQuantity 독립 계산 검증
- 사전 조건: S-001(stock=2, yield=0.9, avgProductionTime=1.0h), O-001(qty=8, RESERVED), O-002(qty=5, RESERVED) JSON 저장
- 실행 1: `OrderController::approveOrder("O-001")`
  - 가용 재고 = 2 − 0 = 2 < 8 → 부족분 = 6, targetProductionQuantity = `ceil(6/0.81)` = 8
- 검증 1: O-001 status = PRODUCING, targetProductionQuantity = 8, productionStartedAt 비어 있지 않음
- 실행 2: `OrderController::approveOrder("O-002")`
  - 가용 재고 = stock(2) − PRODUCING 수량 합계(8) = −6 → 0으로 클램프, 부족분 = 5
  - targetProductionQuantity = `ceil(5/0.81)` = 7
- 검증 2: O-002 status = PRODUCING, targetProductionQuantity = 7
  - O-001의 targetProductionQuantity(8) 불변 확인 (독립 값)

**IT-12** 동일 시료 PRODUCING 2개 — updateProduction 호출 시 front만 처리, queue는 대기 검증
- 사전 조건:
  - S-001(stock=0, avgProductionTime=1.0h)
  - O-001(sampleId="S-001", qty=4, PRODUCING, target=4, producedQty=0, startedAt="2026-06-12 08:00:00") 저장
  - O-002(sampleId="S-001", qty=6, PRODUCING, target=7, producedQty=0, startedAt="2026-06-12 09:00:00") 저장
- 실행: `updateProduction("2026-06-12 11:00:00")`
  - O-001(front) 기준 경과 = 3.0h → ceil(3.0/1.0) = 3, delta = 3
  - O-002(queue) → FIFO 단일 생산라인 규칙으로 처리 대상 아님
- 검증: orders.json 재로드
  - O-001: producedQuantity = 3, status = PRODUCING (target=4 미도달)
  - O-002: producedQuantity = 0, status = PRODUCING (front 아님, 미처리)
  - samples.json 재로드: stock = 3 (0 + 3, O-002 미처리)

**IT-13** 시료 A·B 각각 주문 — getStockStatus 조회 시 각 시료 상태 독립 계산 검증
- 사전 조건:
  - S-A(stock=10), O-A1(sampleId="S-A", qty=7, CONFIRMED), O-A2(sampleId="S-A", qty=2, PRODUCING) 저장
    - S-A 활성 qty 합계 = 9 → 0 < stock(10) 이상 → SURPLUS (stock 10 ≥ 9)
  - S-B(stock=3), O-B1(sampleId="S-B", qty=8, CONFIRMED) 저장
    - S-B 활성 qty 합계 = 8 → stock(3) < 8 → SHORTAGE
- 실행: `SampleController::getStockStatus()`
- 검증:
  - S-A 결과: SURPLUS (`activeQtyBySample["S-A"]` = 9, stock = 10 ≥ 9)
  - S-B 결과: SHORTAGE (`activeQtyBySample["S-B"]` = 8, stock = 3 < 8)
  - 두 시료의 집계가 서로 오염되지 않음

**IT-14** 시료 A 주문 출고 — 시료 A stock만 감소, 시료 B stock 영향 없음 검증
- 사전 조건:
  - S-A(stock=10), O-A(sampleId="S-A", qty=4, CONFIRMED) 저장
  - S-B(stock=7), O-B(sampleId="S-B", qty=3, CONFIRMED) 저장
- 실행: `OrderController::releaseOrder("O-A")`
- 검증:
  - orders.json 재로드: O-A status = RELEASE
  - samples.json 재로드: S-A stock = 6 (10 − 4), S-B stock = 7 (변동 없음)
  - O-B status = CONFIRMED (변동 없음)

---

## 5. E2E 시나리오

시료 등록 → 주문 접수 → 승인 → 출고까지 단일 픽스처에서 연속 실행하여
레이어 간 데이터 일관성을 종단간으로 검증한다.

---

### E2E-01: 재고 충분 경로 (RESERVED → CONFIRMED → RELEASE)

```
[1] 시료 등록
    S-001 (name="산화막시료", avgProductionTime=2.0, yield=0.9, stock=20) 저장
    → samples.json 재로드: stock=20 확인

[2] 주문 접수
    OrderController::createOrder("S-001", "고객A", qty=10)
    → orders.json 재로드: status=RESERVED, id="O-001" 확인

[3] 승인 (가용 재고 20 >= qty 10)
    OrderController::approveOrder("O-001")
    → orders.json 재로드: status=CONFIRMED 확인
    → samples.json 재로드: stock 변동 없음 확인

[4] 출고 처리
    OrderController::releaseOrder("O-001")
    → orders.json 재로드: status=RELEASE 확인
    → samples.json 재로드: stock=10 (20-10) 확인
```

---

### E2E-02: 재고 부족 경로 (RESERVED → PRODUCING → CONFIRMED → RELEASE)

```
[1] 시료 등록
    S-001 (avgProductionTime=1.0h, yield=0.9, stock=3) 저장

[2] 주문 접수
    OrderController::createOrder("S-001", "고객B", qty=10)
    → orders.json 재로드: status=RESERVED 확인

[3] 승인 (가용 재고 3 < qty 10)
    OrderController::approveOrder("O-001")
    → orders.json 재로드:
        status = PRODUCING
        targetProductionQuantity = ceil(7/0.81) = 9
        productionStartedAt != "" 확인
    → samples.json 재로드: stock=3 (즉시 차감 없음) 확인

[4a] 생산 진행 (부분 완료)
    updateProduction("T + 4.0h")  // ceil(4.0/1.0)=4, delta=4
    → orders.json 재로드: producedQuantity=4, status=PRODUCING 유지 확인
    → samples.json 재로드: stock=7 (3+4) 확인

[4b] 생산 완료 (총 생산시간 경과)
    updateProduction("T + 9.0h")  // 총 생산시간 9h 경과
    → orders.json 재로드: status=CONFIRMED, producedQuantity=9 확인
    → samples.json 재로드: stock=12 (3+9) 확인

[5] 출고 처리
    OrderController::releaseOrder("O-001")
    → orders.json 재로드: status=RELEASE 확인
    → samples.json 재로드: stock=2 (12-10) 확인
```

> `T`는 `productionStartedAt` 값이며, `updateProduction`에는 해당 시각에서 지정 시간만큼 경과한 타임스탬프 문자열을 직접 주입한다.

---

## 6. 테스트 커버리지 갭

| 미커버 항목 | 이유 | 대응 방법 |
|---|---|---|
| View 레이어 (`MainView`) | `std::cin` / `std::cout` 의존, 자동화 불가 | Phase 10 수동 시나리오 체크리스트로 대체 |
| `main.cpp` 객체 배선 | Controller·View·Repository 연결 코드 | Phase 9 수동 전체 흐름 확인 |
| 한글 콘솔 출력 (`SetConsoleCP`) | Windows API, 터미널 환경 의존 | Windows 콘솔에서 수동 확인 |
| 생산 대기 큐 UI (FR-040~041) | View 레이어 렌더링 | 수동 확인 대상 |
| `generateOrderId()` 자릿수 오버플로 | O-NNN 3자리 패딩, 1000건 초과 시 O-1000 발생 | 과제 규모상 미발생 가능성 높아 미대응 |
| 동시성 및 대용량 데이터 | 단일 사용자 콘솔 앱, 멀티스레드 없음 | 해당 없음 |
