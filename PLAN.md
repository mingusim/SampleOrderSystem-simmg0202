# PLAN.md — 구현 계획

## 진행 상태 범례
- [ ] 미시작
- [~] 진행 중
- [x] 완료

---

## 개발 사이클 (TDD + Red-Green-Refactor)

```
[RED]      PRD.md 해당 FR 재확인 → 테스트 작성 → 빌드·실행 → 실패 확인 → 사용자 리뷰 → 커밋
[GREEN]    구현 → 테스트 통과 확인 → 사용자 리뷰 → 커밋
[REFACTOR] refactor agent 실행 → 사용자 리뷰 → 커밋
```

### SOLID 준수 포인트

| 원칙 | 적용 |
|------|------|
| S — 단일 책임 | 각 클래스는 하나의 책임만 보유 |
| O — 개방/폐쇄 | 기능 추가 시 기존 클래스 수정 최소화 |
| L — 리스코프 | ISampleRepository / IOrderRepository 인터페이스 준수 |
| I — 인터페이스 분리 | Repository 인터페이스는 필요한 메서드만 노출 |
| D — 의존성 역전 | Controller는 구체 Repository가 아닌 인터페이스에 의존 → gmock 단위 테스트 가능 |

---

## Phase 1. 프로젝트 골격

- [x] `src/` 디렉토리 구조 생성 (model / controller / repository / view)
- [x] `data/` 디렉토리 자동 생성 코드 (`std::filesystem::create_directories`)
- [x] `main.cpp` 진입부 작성 (한글 콘솔 출력 설정)
- [x] nlohmann/json 라이브러리 vcxproj에 연동
- [x] 테스트 프로젝트 vcxproj 생성 — gmock 1.11.0 NuGet 연동, `/utf-8` 옵션 적용
- [x] `test/DummyDataGenerator.h` 작성 — 테스트용 Sample / Order 객체 생성 헬퍼
- [x] 빌드 정상 확인 (메인 + 테스트 프로젝트 모두)

---

## Phase 2. Model 정의

- [x] `model/Sample.h` — id, name, avgProductionTime, yield, stock(초기값 0)
- [x] `model/Order.h` — id, sampleId, customerName, quantity, status, createdAt, productionStartedAt, producedQuantity(초기값 0)
- [x] `OrderStatus` enum 정의 (RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE)

---

## Phase 3. Repository 인터페이스 + JSON 구현

**[RED]**
- [x] `ISampleRepository` 추상 인터페이스 정의 (findById, findAll, save, remove)
- [x] `IOrderRepository` 추상 인터페이스 정의
- [x] `test/RepositoryTestFixture.h` 작성 — 임시 `data/` 경로 설정, TearDown 시 파일 삭제
- [x] Repository 통합 테스트 작성 (RepositoryTestFixture 상속, 왕복 검증)
  - save → findAll → 값 일치 확인
  - save → findById → 값 일치 확인
  - remove → findAll → 없음 확인
- [x] 빌드·실행 → 테스트 실패 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [x] `SampleRepository : ISampleRepository` 구현 (samples.json CRUD)
- [x] `OrderRepository : IOrderRepository` 구현 (orders.json CRUD)
- [x] 테스트 통과 확인

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [x] refactor agent 실행

→ **사용자 리뷰 후 커밋**

---

## Phase 4. 시료 관리 (FR-010~012)

**[RED]**
- [x] `MockSampleRepository` (gmock) 작성
- [x] `SampleController` 단위 테스트 작성
  - FR-010: ID 중복 시 등록 거부
  - FR-010: 수율 범위 초과(< 0.01 또는 > 1.0) 시 등록 거부
  - FR-010: 정상 등록 → save 1회 호출, stock 초기값 0
  - FR-011: findAll 결과 반환 확인
  - FR-012: 이름 부분 일치 검색 결과 확인
  - FR-012: ID 검색 — 존재하는 경우 / 존재하지 않는 경우
- [x] 빌드·실행 → 테스트 실패 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [x] `SampleController(ISampleRepository&)` 구현
  - FR-010: 시료 등록 (ID 중복 불가, stock 초기값 0, 수율 범위 검증 0.01~1.0)
  - FR-011: 전체 목록 조회
  - FR-012: 이름 부분 일치 검색
  - FR-012: ID 검색 (`findById` 위임)
- [x] `MainView` — 시료 관리 서브메뉴 UI (3.시료 검색 → ID/이름 선택 서브메뉴)
- [x] 테스트 통과 확인

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [x] refactor agent 실행 (readMenuChoice 추출, avgTime→avgProductionTime)

→ **사용자 리뷰 후 커밋**

---

## Phase 5. 주문 접수·승인·거절 (FR-020~024)

**[RED]**
- [x] `MockOrderRepository` (gmock) 작성 (MockSampleRepository는 Phase 4 재사용)
- [x] `OrderController` 단위 테스트 작성 (13개)
  - FR-020: 미등록 시료 ID로 주문 시 거부
  - FR-020: 정상 주문 생성 → 상태 RESERVED, save 1회 호출
  - FR-020: 순번 ID — 첫 주문 O-001, 기존 최대 O-003 시 O-004
  - FR-021: RESERVED 주문 목록 반환 확인
  - FR-022: 가용 재고 >= quantity → CONFIRMED 전환
  - FR-022: 가용 재고 < quantity → PRODUCING 전환
  - FR-023: 생산량 계산식 검증 (정상 / 가용 재고 음수 / 재고 0 케이스)
  - FR-024: RESERVED → REJECTED 전환
- [x] 빌드·실행 → 테스트 실패 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [x] `OrderController(ISampleRepository&, IOrderRepository&)` 구현
  - FR-020~024 구현, generateOrderId() 순번 기반
- [x] `MainView` — 주문 서브메뉴 UI
- [x] 테스트 통과 확인 (13개)

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [x] refactor agent 실행

→ **사용자 리뷰 후 커밋**

---

## Phase 6. 모니터링 (FR-030~032)

**[RED]**
- [x] `model/Order.h` — `OrderStats` 구조체 추가
- [x] `model/Sample.h` — `StockStatus` enum, `SampleStockInfo` 구조체 추가
- [x] `OrderController` 신규 메서드 선언: `getOrderStats()`, `getActiveOrders()`
- [x] `SampleController` 신규 메서드 선언: `getStockStatus(activeOrders)`
- [x] `OrderControllerTest` 모니터링 테스트 추가 (3개)
  - FR-030: 상태별 주문 건수 집계, REJECTED 제외 확인
  - FR-030: 주문 없을 때 모든 카운트 0
  - FR-030: getActiveOrders — CONFIRMED + PRODUCING만 반환
- [x] `SampleControllerTest` 재고 상태 테스트 추가 (4개)
  - FR-032: stock >= activeQty → SURPLUS
  - FR-032: 0 < stock < activeQty → SHORTAGE
  - FR-032: stock == 0 → DEPLETED
  - FR-032: activeOrders 없고 stock > 0 → SURPLUS
- [ ] 빌드·실행 → 테스트 실패 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [x] `OrderController::getOrderStats()` 구현
- [x] `OrderController::getActiveOrders()` 구현
- [x] `SampleController::getStockStatus()` 구현
- [x] `MainView` — 모니터링 서브메뉴 UI (상태별 주문 건수 / 시료별 재고 현황)
- [x] 테스트 통과 확인

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [ ] refactor agent 실행

→ **사용자 리뷰 후 커밋**

---

## Phase 7. 생산라인 (FR-040~042)

### 설계 결정

- `updateProduction(const std::string& now)` — 테스트 시 시간 주입을 위해 현재 시각을 파라미터로 받음
  - View는 `currentTimestamp()` 결과를 전달
  - 테스트는 고정된 타임스탬프 전달 → 결정적(deterministic) 검증 가능
- `getCurrentProduction()` — PRODUCING 주문 중 가장 먼저 시작된 1건 반환 (FIFO)
- `getProductionQueue()` — 모든 PRODUCING 주문 목록 (생산 시작 시각 오름차순)
- 새 반환 타입 `ProductionInfo` 구조체 (`model/Order.h`에 추가)

### 신규 타입

```cpp
struct ProductionInfo {
    Order  order;
    Sample sample;
    double totalProductionHours;  // avgProductionTime × targetProductionQuantity
};
```

**[RED]**
- [ ] `model/Order.h` — `ProductionInfo` 구조체 추가
- [ ] `OrderController` 신규 메서드 선언
  - `void updateProduction(const std::string& now)`
  - `std::optional<ProductionInfo> getCurrentProduction()`
  - `std::vector<ProductionInfo> getProductionQueue()`
- [ ] `OrderControllerTest` 생산라인 테스트 추가 (7개)
  - FR-042: 경과 시간 < 1단위 → delta=0, save 없음
  - FR-042: 경과 시간 = 2.5h, avgTime=1.0h → delta=2, stock+=2, producedQty+=2
  - FR-042: delta가 targetQty 초과 → targetQty로 클램프
  - FR-042: 총 생산시간 경과 → PRODUCING → CONFIRMED 전환
  - FR-040: PRODUCING 주문 있음 → getCurrentProduction 반환
  - FR-040: PRODUCING 주문 없음 → nullopt 반환
  - FR-041: PRODUCING 주문 여러 개 → getProductionQueue 전체 반환
- [ ] 빌드·실행 → 테스트 실패(링커 에러) 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [ ] `OrderController::updateProduction()` 구현 (elapsed 계산, delta 적용, CONFIRMED 전환)
- [ ] `OrderController::getCurrentProduction()` 구현
- [ ] `OrderController::getProductionQueue()` 구현
- [ ] `MainView` — 생산라인 서브메뉴 UI (현재 생산 / 대기 큐 / 상태 갱신)
- [ ] 테스트 통과 확인

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [ ] refactor agent 실행

→ **사용자 리뷰 후 커밋**

---

## Phase 8. 출고 처리 (FR-050~051)

**[RED]**
- [ ] 출고 처리 단위 테스트 작성
  - FR-051: CONFIRMED → RELEASE 전환
  - FR-051: 출고 후 stock -= quantity 확인
- [ ] 빌드·실행 → 테스트 실패 확인

→ **사용자 리뷰 후 커밋**

**[GREEN]**
- [ ] FR-050: CONFIRMED 주문 목록 표시
- [ ] FR-051: 출고 처리 구현
- [ ] `MainView` — 출고 처리 서브메뉴 UI
- [ ] 테스트 통과 확인

→ **사용자 리뷰 후 커밋**

**[REFACTOR]**
- [ ] refactor agent 실행

→ **사용자 리뷰 후 커밋**

---

## Phase 9. 메인 메뉴 통합 (FR-001~003)

- [ ] 시스템 시작 시 메인 메뉴 표시
- [ ] 전체 시료 수, 상태별 주문 수 요약 표시 (FR-003)
- [ ] 전체 메뉴 흐름 통합 확인

→ **사용자 리뷰 후 커밋**

---

## Phase 10. validator agent 전체 검증

- [ ] validator agent 실행 — 전체 FR PASS/FAIL 보고
- [ ] FAIL 항목 수정 후 재검증
