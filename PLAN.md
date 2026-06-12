# PLAN.md — 구현 계획

## 진행 상태 범례
- [ ] 미시작
- [~] 진행 중
- [x] 완료

---

## Phase 1. 프로젝트 골격

- [ ] `src/` 디렉토리 구조 생성 (model / controller / repository / view)
- [ ] `data/` 디렉토리 자동 생성 코드 (`std::filesystem::create_directories`)
- [ ] `main.cpp` 진입부 작성 (한글 콘솔 출력 설정)
- [ ] nlohmann/json 라이브러리 vcxproj에 연동
- [ ] 빌드 정상 확인

---

## Phase 2. Model 정의

- [ ] `model/Sample.h` — id, name, avgProductionTime, yield, stock(초기값 0)
- [ ] `model/Order.h` — id, sampleId, customerName, quantity, status, createdAt
- [ ] `OrderStatus` enum 정의 (RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE)

---

## Phase 3. Repository (JSON CRUD)

- [ ] `repository/SampleRepository.h/.cpp` — samples.json 읽기/쓰기, CRUD
- [ ] `repository/OrderRepository.h/.cpp` — orders.json 읽기/쓰기, CRUD
- [ ] 앱 재시작 후 데이터 유지 확인 (영속성 검증)

---

## Phase 4. 시료 관리 (FR-010~012)

- [ ] `SampleController` — 시료 등록, 목록 조회, 이름 검색
  - FR-010: 등록 (ID 중복 불가, stock 초기값 0)
  - FR-011: 전체 목록 + 현재 재고 표시
  - FR-012: 이름 부분 일치 검색
- [ ] `MainView` — 시료 관리 서브메뉴 UI

---

## Phase 5. 주문 접수·승인·거절 (FR-020~024)

- [ ] `OrderController` — 주문 생성, 승인, 거절
  - FR-020: 주문 생성 (미등록 시료 ID 거부, 즉시 RESERVED)
  - FR-022: 승인 — 가용 재고 계산 후 CONFIRMED / PRODUCING 분기
  - FR-023: 생산량 계산식 구현
    ```
    가용 재고  = stock - (CONFIRMED + PRODUCING 주문 수량 합계)
    부족분     = quantity - max(0, 가용 재고)
    실 생산량  = ceil(부족분 / (수율 × 0.9))
    총 생산시간 = 평균생산시간 × 실 생산량
    ```
  - FR-024: 거절 — 즉시 REJECTED
- [ ] `MainView` — 주문 서브메뉴 UI

---

## Phase 6. 모니터링 (FR-030~032)

- [ ] 상태별 주문 건수 표시 (RESERVED / PRODUCING / CONFIRMED / RELEASE, REJECTED 제외)
- [ ] 시료별 재고 수량 표시
- [ ] 재고 상태 표기
  - 여유: stock >= (CONFIRMED + PRODUCING 주문 수량 합계)
  - 부족: 0 < stock < (CONFIRMED + PRODUCING 주문 수량 합계)
  - 고갈: stock == 0
- [ ] `MainView` — 모니터링 서브메뉴 UI

---

## Phase 7. 생산라인 (FR-040~042)

- [ ] 생산 대기 큐 (FIFO) 구현
- [ ] FR-040: 현재 생산 중인 시료 정보 표시
- [ ] FR-041: 생산 대기 큐 목록 표시
- [ ] FR-042: 모든 메뉴 진입 시 경과 시간 체크
  - productionStartedAt 기준 경과 시간 계산
  - 총 생산량 = floor(경과시간 / avgProductionTime), 실 생산량 초과 불가
  - delta = 총 생산량 - producedQuantity → 재고 += delta, producedQuantity += delta
  - 총 생산시간 경과 시 PRODUCING → CONFIRMED 자동 전환
- [ ] `MainView` — 생산라인 서브메뉴 UI

---

## Phase 8. 출고 처리 (FR-050~051)

- [ ] FR-050: CONFIRMED 주문 목록 표시
- [ ] FR-051: 출고 처리 — CONFIRMED → RELEASE, 주문 수량만큼 재고 차감
- [ ] `MainView` — 출고 처리 서브메뉴 UI

---

## Phase 9. 메인 메뉴 요약 (FR-001~003)

- [ ] 시스템 시작 시 메인 메뉴 표시
- [ ] 전체 시료 수, 상태별 주문 수 요약 표시
- [ ] 전체 메뉴 흐름 통합 확인

---

## Phase 10. 테스트

- [ ] gmock 기반 Controller 단위 테스트
  - 가용 재고 계산 (정상 / 가용 재고 음수 / 재고 0)
  - 주문 승인 분기 (CONFIRMED / PRODUCING)
  - 생산량 계산식 검증
  - 상태 전이 규칙 검증
- [ ] Repository 통합 테스트 (JSON 파일 왕복 검증)
- [ ] AGENT_VALIDATOR 기반 전체 기능 검증
