# TEST_PLAN.md — 반도체 시료 생산주문관리 시스템

> 생성 기준일: 2026-06-12  
> 테스트 프레임워크: Google Mock 1.11.0  
> 상태 범례: GREEN = 구현 완료·통과 / RED = 미구현·실패 예정

---

## Phase 3. Repository 통합 테스트

### SampleRepositoryTest

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| SaveAndFindAll_ReturnsSavedSample | 영속성 | 임시 디렉토리 생성 | save 후 findAll → 저장된 시료 1건 반환, id/name 일치 | GREEN |
| SaveAndFindById_ReturnsCorrectSample | 영속성 | 임시 디렉토리 생성 | save 후 findById → 정확한 시료 반환, yield 값 일치 | GREEN |
| FindById_NotFound_ReturnsNullopt | 영속성 | 빈 저장소 | 미존재 ID 조회 → nullopt 반환 | GREEN |
| Remove_RemovesSample | 영속성 | 시료 1건 저장 | remove 후 findAll → 빈 목록 반환 | GREEN |
| Save_Upsert_UpdatesExistingSample | 영속성 | 시료 1건 저장 | 동일 ID로 재저장 시 upsert — findAll 결과 1건, name/stock 갱신 확인 | GREEN |

### OrderRepositoryTest

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| SaveAndFindAll_ReturnsSavedOrder | 영속성 | 임시 디렉토리 생성 | save 후 findAll → 저장된 주문 1건, id/status 일치 | GREEN |
| SaveAndFindById_ReturnsCorrectOrder | 영속성 | 임시 디렉토리 생성 | save 후 findById → 정확한 주문 반환, quantity 일치 | GREEN |
| FindById_NotFound_ReturnsNullopt | 영속성 | 빈 저장소 | 미존재 ID 조회 → nullopt 반환 | GREEN |
| Remove_RemovesOrder | 영속성 | 주문 1건 저장 | remove 후 findAll → 빈 목록 반환 | GREEN |
| Save_Upsert_UpdatesExistingOrder | 영속성 | 주문 1건 저장 | 동일 ID로 status 변경 후 재저장 → findAll 1건, CONFIRMED 상태 확인 | GREEN |
| Persistence_ReloadAfterSave_RetainsData | 영속성 | 주문 1건 저장 (PRODUCING) | 동일 경로로 새 인스턴스 생성(재로드) → status/producedQuantity/productionStartedAt 모두 일치 | GREEN |

---

## Phase 4. 시료 관리 (FR-010~012)

### SampleControllerTest

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| Register_YieldTooLow_ReturnsFalse | FR-010 | MockRepo 주입 | yield=0.009 → findById 호출 없이 false 반환 (수율 범위 검증 우선) | GREEN |
| Register_YieldTooHigh_ReturnsFalse | FR-010 | MockRepo 주입 | yield=1.001 → findById 호출 없이 false 반환 | GREEN |
| Register_YieldBoundary_Low_Succeeds | FR-010 | findById → nullopt | yield=0.01(하한 경계) → save 1회 호출, true 반환 | GREEN |
| Register_YieldBoundary_High_Succeeds | FR-010 | findById → nullopt | yield=1.0(상한 경계) → save 1회 호출, true 반환 | GREEN |
| Register_DuplicateId_ReturnsFalse | FR-010 | findById → 기존 시료 반환 | 중복 ID 등록 시도 → save 호출 없이 false 반환 | GREEN |
| Register_Success_CallsSaveOnce | FR-010 | findById → nullopt | 정상 등록 → save 정확히 1회 호출, true 반환 | GREEN |
| Register_Success_StockIsZero | FR-010 | findById → nullopt | 정상 등록 시 stock 필드 = 0으로 save 호출 확인 | GREEN |
| GetAllSamples_ReturnsFindAllResult | FR-011 | findAll → 시료 1건 목록 | getAllSamples → findAll 결과 그대로 반환, id 일치 | GREEN |
| SearchByName_DelegatesToRepository | FR-012 | findByName("반도체") → 시료 1건 | searchByName 호출 → findByName 위임, 결과 id 일치 | GREEN |
| FindById_Found_ReturnsSample | FR-012 | findById → 시료 반환 | findById("S-001") → id/name 일치하는 optional 반환 | GREEN |
| FindById_NotFound_ReturnsNullopt | FR-012 | findById → nullopt | findById("S-999") → nullopt 반환 | GREEN |

---

## Phase 5. 주문 접수·승인·거절 (FR-020~024)

### OrderControllerTest

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| CreateOrder_UnknownSampleId_ReturnsFalse | FR-020 | findById("S-999") → nullopt | 미등록 시료 ID 주문 → save 호출 없이 false 반환 | GREEN |
| CreateOrder_ValidSample_StatusReserved | FR-020 | 시료 존재, 기존 주문 없음 | 정상 주문 생성 → status=RESERVED 로 save 1회, true 반환 | GREEN |
| CreateOrder_NoExistingOrders_IdIsO001 | FR-020 | 기존 주문 목록 비어 있음 | 첫 번째 주문의 id = "O-001" 확인 | GREEN |
| CreateOrder_ExistingMaxO003_IdIsO004 | FR-020 | 기존 최대 순번 O-003 존재 | 다음 주문 id = "O-004" 확인 (순번 자동 증가) | GREEN |
| GetPendingOrders_DelegatesToFindByStatus | FR-021 | findByStatus(RESERVED) → 1건 | getPendingOrders → RESERVED 목록 반환, id 일치 | GREEN |
| ApproveOrder_NotFound_ReturnsFalse | FR-022 | findById("O-999") → nullopt | 미존재 주문 승인 → false 반환 | GREEN |
| ApproveOrder_SufficientStock_BecomesConfirmed | FR-022 | stock=10, qty=5, 기존 활성 주문 없음 | 가용 재고 >= quantity → CONFIRMED 로 save, true 반환 | GREEN |
| ApproveOrder_InsufficientStock_BecomesProducing | FR-022 | stock=3, qty=10, 기존 활성 주문 없음 | 가용 재고 < quantity → PRODUCING 으로 save, true 반환 | GREEN |
| CalcProduction_Normal_CeilApplied | FR-023 | stock=3, qty=10, yield=0.9 | 부족분=7 → ceil(7/0.81)=9, targetProductionQuantity=9 저장 | GREEN |
| CalcProduction_NegativeAvailable_UsesZero | FR-023 | stock=0, 기존 CONFIRMED qty=5, 신규 qty=3 | 가용 재고 음수 → max(0,−5)=0 클립 → shortage=3 → ceil(3/0.81)=4 저장 | GREEN |
| CalcProduction_ZeroStock_FullOrderQty | FR-023 | stock=0, qty=5, 기존 주문 없음 | shortage=5 → ceil(5/0.81)=7, targetProductionQuantity=7 저장 | GREEN |
| RejectOrder_NotFound_ReturnsFalse | FR-024 | findById("O-999") → nullopt | 미존재 주문 거절 → false 반환 | GREEN |
| RejectOrder_StatusBecomesRejected | FR-024 | RESERVED 주문 존재 | rejectOrder → status=REJECTED 로 save, true 반환 | GREEN |

---

## Phase 6. 모니터링 (FR-030~032)

### OrderControllerTest — 모니터링 추가

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| GetOrderStats_VariousStatuses_CountsExcludeRejected | FR-030 | RESERVED×2, PRODUCING×1, CONFIRMED×1, RELEASE×1, REJECTED×2 | reserved=2, producing=1, confirmed=1, release=1 (REJECTED 집계 제외) | RED |
| GetOrderStats_NoOrders_AllZero | FR-030 | 주문 목록 비어 있음 | 모든 카운트 = 0 | RED |
| GetActiveOrders_ReturnsOnlyConfirmedAndProducing | FR-030 | RESERVED/CONFIRMED/PRODUCING/REJECTED/RELEASE 각 1건 | 결과 2건 — CONFIRMED·PRODUCING 만 포함 | RED |

### SampleControllerTest — 재고 상태 추가

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| GetStockStatus_Surplus_WhenStockGteActiveQty | FR-032 | stock=10, CONFIRMED qty=5, PRODUCING qty=3 | activeQty=8, stock≥activeQty → StockStatus::SURPLUS | RED |
| GetStockStatus_Shortage_WhenStockBetweenZeroAndActiveQty | FR-032 | stock=3, CONFIRMED qty=5, PRODUCING qty=3 | 0 < stock=3 < activeQty=8 → StockStatus::SHORTAGE | RED |
| GetStockStatus_Depleted_WhenStockIsZero | FR-032 | stock=0, 활성 주문 없음 | stock=0 → StockStatus::DEPLETED | RED |
| GetStockStatus_Surplus_WhenNoActiveOrders | FR-032 | stock=5, 활성 주문 없음 | activeQty=0, stock≥0 → StockStatus::SURPLUS | RED |

---

## Phase 7. 생산라인 (FR-040~042) — 테스트 미작성

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| (미작성) delta 정상 계산 | FR-042 | 경과 시간 일부 진행 | delta = floor(경과시간/avgTime) − producedQuantity, delta > 0 시 stock/producedQuantity 갱신 | RED |
| (미작성) 실 생산량 초과 방지 | FR-042 | 경과 시간 초과 | delta가 targetProductionQuantity 초과 불가 | RED |
| (미작성) 총 생산시간 경과 시 CONFIRMED 전환 | FR-042 | totalProductionTime 이상 경과 | PRODUCING → CONFIRMED 자동 전환 | RED |

---

## Phase 8. 출고 처리 (FR-050~051) — 테스트 미작성

| 테스트명 | 대상 FR | 사전조건 | 검증 내용 | 상태 |
|----------|---------|----------|-----------|------|
| (미작성) ReleaseOrder_BecomesRelease | FR-051 | CONFIRMED 주문 존재 | 출고 처리 → status=RELEASE 전환 확인 | RED |
| (미작성) ReleaseOrder_DeductsStock | FR-051 | CONFIRMED 주문 존재, 재고 충분 | 출고 후 stock -= quantity 확인 | RED |

---

## 전체 진행 현황 요약

| Phase | 내용 | 테스트 수 | GREEN | RED |
|-------|------|-----------|-------|-----|
| Phase 3 | Repository 통합 테스트 | 11 | 11 | 0 |
| Phase 4 | 시료 관리 (FR-010~012) | 11 | 11 | 0 |
| Phase 5 | 주문 접수·승인·거절 (FR-020~024) | 13 | 13 | 0 |
| Phase 6 | 모니터링 (FR-030~032) | 7 | 0 | 7 |
| Phase 7 | 생산라인 (FR-040~042) | 3 | 0 | 3 |
| Phase 8 | 출고 처리 (FR-050~051) | 2 | 0 | 2 |
| **합계** | | **47** | **35** | **12** |

> Phase 6의 RED 테스트는 코드가 작성되어 있으나 구현(`getOrderStats`, `getActiveOrders`, `getStockStatus`)이 완료되지 않은 상태입니다.  
> Phase 7·8의 테스트는 아직 작성되지 않았습니다.
