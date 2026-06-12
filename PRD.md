# PRD.md — 반도체 시료 생산주문관리 시스템

## 배경

반도체 회사 S-Semi는 다양한 시료(Sample)를 연구소·팹리스·대학에 납품한다.
주문 급증으로 엑셀·메모장 관리의 한계에 도달 → 콘솔 기반 통합 관리 시스템 개발.

---

## 시스템 개요

- **실행 방식**: 콘솔(터미널) 기반, 담당자가 직접 명령 입력
- **생산 라인**: 시료 1개를 순차 생산하는 설비 시뮬레이션 (FIFO 큐)
- **영속성**: 앱 재시작 후에도 데이터 유지 (JSON 파일)

---

## 주문 상태 정의

| 상태 | 의미 |
|------|------|
| RESERVED | 주문 접수 |
| REJECTED | 주문 거절 (모니터링 제외) |
| PRODUCING | 승인 완료, 재고 부족으로 생산 중 |
| CONFIRMED | 승인 완료 또는 생산 완료, 출고 대기 중 |
| RELEASE | 출고 완료 |

---

## 기능 요구사항

### 1. 메인 메뉴

**FR-001** 시스템 시작 시 메인 메뉴를 표시한다.

**FR-002** 메인 메뉴는 다음 항목을 포함한다.
- 시료 관리
- 주문 (접수 / 승인 / 거절)
- 모니터링
- 생산라인
- 출고 처리
- 종료

**FR-003** 메인 메뉴 화면에서 전체 시료 수, 상태별 주문 수 요약을 표시한다.

---

### 2. 시료 관리

**FR-010** 시료(Sample)를 시스템에 등록한다.
- 입력 값: 시료 ID, 이름, 평균 생산시간(h/개), 수율(0.01~1.0)
- ID 중복 등록 불가

**FR-011** 등록된 전체 시료 목록을 조회한다.
- 현재 재고 수량 함께 표시

**FR-012** 시료를 검색한다.
- **ID 검색**: 정확히 일치하는 시료 반환
- **이름 검색**: 부분 일치 검색 허용

> **수율 정의**: 정상 시료 수 / 총 생산 시료 수 (예: 100개 생산 중 90개 정상 = 0.9)

---

### 3. 주문

**FR-020** 고객이 시료를 요청하면 주문을 생성한다.
- 입력 값: 시료 ID, 고객명, 주문 수량
- 등록되지 않은 시료 ID는 거부
- 생성 즉시 상태: RESERVED

**FR-021** RESERVED 주문 목록을 표시한다.

**FR-022** 특정 주문을 승인한다. 승인 시 재고 상황에 따라 자동 분기한다.
- **재고 충분** (가용 재고 >= quantity): 즉시 CONFIRMED 전환
- **재고 부족** (가용 재고 < quantity): 부족분만 생산 라인에 자동 등록, PRODUCING 전환 (재고 즉시 차감 없음)

**FR-023** 생산량 계산식
```
가용 재고 = stock - (해당 시료의 CONFIRMED + PRODUCING 주문 수량 합계)
부족분    = quantity - max(0, 가용 재고)
실 생산량 = ceil(부족분 / (수율 × 0.9))
총 생산시간 = 평균생산시간 × 실 생산량
```

**FR-024** 특정 RESERVED 주문을 거절한다. 즉시 REJECTED 전환.

---

### 4. 모니터링

**FR-030** 상태별 주문 수를 표시한다.
- RESERVED / PRODUCING / CONFIRMED / RELEASE 건수 표시
- REJECTED는 제외

**FR-031** 시료별 현재 재고 수량을 표시한다.

**FR-032** 주문 대비 재고 상태를 함께 표기한다.

| 상태 표기 | 조건 |
|----------|------|
| 여유 | stock >= (해당 시료의 CONFIRMED + PRODUCING 주문 수량 합계) |
| 부족 | 0 < stock < (해당 시료의 CONFIRMED + PRODUCING 주문 수량 합계) |
| 고갈 | stock == 0 |

---

### 5. 생산라인

**FR-040** 현재 생산 중인 시료 정보를 표시한다.
- 주문 정보, 예상 총 생산시간 등 포함

**FR-041** 생산 대기 큐를 표시한다.
- 스케줄링 전략: **FIFO**

**FR-042** 모든 메뉴 진입 시 경과 시간을 체크하여 생산 상태를 갱신한다.
- productionStartedAt 기준으로 경과 시간 계산
- 총 생산량 = floor(경과시간 / avgProductionTime), 단 실 생산량 초과 불가
- delta = 총 생산량 - producedQuantity
- delta > 0이면 재고 += delta, producedQuantity += delta
- 총 생산시간 경과 시 자동으로 PRODUCING → CONFIRMED 전환

---

### 6. 출고 처리

**FR-050** CONFIRMED 상태의 주문 목록을 표시한다.

**FR-051** 특정 주문의 출고를 처리한다.
- CONFIRMED → RELEASE 전환
- 주문 수량만큼 재고 차감

---

## 데이터 모델

### Sample

| 필드 | 타입 | 설명 |
|------|------|------|
| id | string | 고유 식별자 |
| name | string | 시료 이름 |
| avgProductionTime | double | 개당 평균 생산시간(h) |
| yield | double | 수율 (0.01~1.0) |
| stock | int | 현재 재고 수량 (초기값: 0) |

### Order

| 필드 | 타입 | 설명 |
|------|------|------|
| id | string | 고유 식별자 |
| sampleId | string | 연결된 시료 ID |
| customerName | string | 고객명 |
| quantity | int | 주문 수량 |
| status | enum | RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE |
| createdAt | string | 주문 접수 일시 (YYYY-MM-DD HH:MM:SS) |
| productionStartedAt | string | 생산 시작 일시 (YYYY-MM-DD HH:MM:SS, PRODUCING 전환 시 기록) |
| producedQuantity | int | 재고에 반영된 누적 생산 수량 (초기값: 0, 메뉴 진입마다 delta만큼 갱신) |

---

## 비기능 요구사항

| 항목 | 요건 |
|------|------|
| 영속성 | 앱 재시작 후 데이터 유지 (JSON 파일) |
| 언어 | C++20 |
| 빌드 | MSBuild (Visual Studio .vcxproj) |
| 인코딩 | UTF-8 소스 (/utf-8), 콘솔 UTF-8 출력 |
| 테스트 | Google Mock (gmock 1.11.0) |
| 코드 품질 | MVC 레이어 분리, CleanCode 준수 |
| 이력 | 기능 단위 Git 커밋 |
