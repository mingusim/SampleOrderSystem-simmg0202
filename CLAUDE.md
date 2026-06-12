# CLAUDE.md — SampleOrderSystem

## 프로젝트 개요

반도체 시료 생산주문관리 시스템 (S-Semi 사내 툴)
C++20 콘솔 기반 애플리케이션. MVC 패턴 + JSON 파일 영속성.

---

## 빌드 환경

- **언어**: C++20
- **빌드**: MSBuild (Visual Studio .vcxproj)
- **컴파일러**: MSVC (v145)
- **외부 라이브러리**: nlohmann/json
- **UTF-8**: 모든 구성에 `/utf-8` 컴파일 옵션 적용 (vcxproj에 설정 완료)

### 빌드 방법

Visual Studio에서 솔루션 파일(`SampleOrderSystem-simmg0202.slnx`) 열고 빌드.

> `/utf-8` 없으면 한글 소스가 CP949로 해석되어 컴파일 에러 발생

### 런타임 한글 출력

`main.cpp` 진입부에 반드시 추가:

```cpp
#ifdef _WIN32
#include <windows.h>
SetConsoleCP(65001);
SetConsoleOutputCP(65001);
#endif
```

---

## 아키텍처

### 레이어 구조

```
src/
├── main.cpp
├── model/          ← 순수 데이터 구조 (로직 없음)
│   ├── Sample.h
│   └── Order.h
├── controller/     ← 비즈니스 로직
│   ├── SampleController.h/.cpp
│   └── OrderController.h/.cpp
├── repository/     ← JSON 파일 CRUD (영속성)
│   ├── SampleRepository.h/.cpp
│   └── OrderRepository.h/.cpp
└── view/           ← 콘솔 I/O만 담당
    └── MainView.h/.cpp
```

### 의존 방향

```
View → Controller → Repository → Model
                              ↘ (Model 소유)
```

- View는 Controller만 호출. Repository/Model 직접 접근 금지
- Controller는 Repository를 통해서만 데이터 접근
- Model은 어떤 레이어도 의존하지 않음

### 데이터 저장 경로

```
data/
├── samples.json
└── orders.json
```

실행 파일 기준 상대 경로. `std::filesystem::create_directories("data")` 로 자동 생성.

---

## 도메인 규칙

### 주문 상태 머신

```
RESERVED ──(승인, 재고 충분)──▶ CONFIRMED ──(출고)──▶ RELEASE
         ──(승인, 재고 부족)──▶ PRODUCING ──(생산 완료)──▶ CONFIRMED
         ──(거절)────────────▶ REJECTED
```

- REJECTED는 모니터링에서 제외
- 상태 전이는 반드시 Controller에서만 수행

### 생산량 계산

```
가용 재고  = stock - (CONFIRMED + PRODUCING 주문 수량 합계)
부족분     = quantity - max(0, 가용 재고)
실 생산량  = ceil(부족분 / (수율 × 0.9))
총 생산시간 = 평균생산시간 × 실 생산량
```

### 생산라인 스케줄링

- FIFO 큐 사용
- 한 번에 1개 주문 생산

### 재고 상태 표기

| 상태 | 조건 |
|------|------|
| 여유 | stock >= (CONFIRMED + PRODUCING 주문 수량 합계) |
| 부족 | 0 < stock < (CONFIRMED + PRODUCING 주문 수량 합계) |
| 고갈 | stock == 0 |

---

## 코딩 컨벤션

- 클래스명: PascalCase (`SampleController`)
- 멤버 변수: trailing underscore (`samples_`)
- 함수명: camelCase (`findById`)
- 상수/enum: UPPER_CASE (`OrderStatus::CONFIRMED`)
- 헤더 가드: `#pragma once`
- 주석: WHY가 명확할 때만 작성. WHAT 설명 주석 금지

---

## 테스트 방침

- 단위 테스트: Controller 레이어 로직 검증
- 통합 테스트: Repository → JSON 파일 왕복 검증
- 테스트 데이터: DummyDataGenerator 활용
- 테스트 프레임워크: 별도 지정 없으면 직접 assert 또는 간단한 main 기반 테스트

---

## 하네스(Harness) 운영 지침

- 기능 구현 전 PRD.md 요구사항 재확인
- 각 기능 완료 후 빌드 → 실행 확인
- 커밋은 사용자가 직접 수행 (커밋 컨벤션: `GIT_CONVENTION.md` 참고)

---

## 금지 사항

- View에서 데이터 직접 수정
- Controller에서 콘솔 출력 (`std::cout`)
- Model에 비즈니스 로직 추가
- 빌드 실패 상태로 커밋
- Opus 모델 사용 (과제 규정: Sonnet / Effort: Medium만 허용)
