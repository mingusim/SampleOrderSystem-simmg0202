#include "view/MainView.h"
#include "utils/TimeUtils.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

static std::string statusLabel(OrderStatus s) {
    switch (s) {
        case OrderStatus::RESERVED:  return "접수대기";
        case OrderStatus::PRODUCING: return "생산중";
        case OrderStatus::CONFIRMED: return "승인완료";
        case OrderStatus::REJECTED:  return "거절";
        case OrderStatus::RELEASE:   return "출고완료";
        default:                     return "?";
    }
}

MainView::MainView(SampleController& sampleController, OrderController& orderController)
    : sampleCtrl_(sampleController), orderCtrl_(orderController) {}

int MainView::readMenuChoice() {
    int choice = 0;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void MainView::run() {
    while (true) {
        orderCtrl_.updateProduction(TimeUtils::currentTimestamp());  // FR-042
        showMainMenu();
        switch (readMenuChoice()) {
            case 1: handleSampleMenu();      break;
            case 2: handleOrderMenu();       break;
            case 3: handleMonitorMenu();     break;
            case 4: handleProductionMenu();  break;
            case 0: return;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void MainView::showMainMenu() {
    std::cout << "\n========== 시료 생산주문관리 시스템 ==========\n";
    std::cout << "  1. 시료 관리\n";
    std::cout << "  2. 주문 관리\n";
    std::cout << "  3. 모니터링\n";
    std::cout << "  4. 생산라인\n";
    std::cout << "  0. 종료\n";
    std::cout << "선택: ";
}

void MainView::handleSampleMenu() {
    while (true) {
        std::cout << "\n----- 시료 관리 -----\n";
        std::cout << "  1. 시료 등록\n";
        std::cout << "  2. 전체 목록 조회\n";
        std::cout << "  3. 시료 검색\n";
        std::cout << "  0. 뒤로\n";
        std::cout << "선택: ";
        const int choice = readMenuChoice();

        switch (choice) {
            case 0: return;
            case 1: {
                std::string id, name;
                double avgProductionTime, yield;
                std::cout << "ID: "; std::getline(std::cin, id);
                std::cout << "이름: "; std::getline(std::cin, name);
                std::cout << "평균 생산시간(h): "; std::cin >> avgProductionTime;
                std::cout << "수율(0.01~1.0): "; std::cin >> yield;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (sampleCtrl_.registerSample(id, name, avgProductionTime, yield))
                    std::cout << "등록 완료.\n";
                else
                    std::cout << "등록 실패 (중복 ID 또는 수율 범위 오류).\n";
                break;
            }
            case 2: printSampleList(sampleCtrl_.getAllSamples()); break;
            case 3: handleSearchMenu(); break;
            default: std::cout << "잘못된 입력입니다.\n"; break;
        }
    }
}

void MainView::handleSearchMenu() {
    std::cout << "\n  검색 방법:\n";
    std::cout << "    1. ID로 검색\n";
    std::cout << "    2. 이름으로 검색\n";
    std::cout << "    0. 취소\n";
    std::cout << "  선택: ";
    const int type = readMenuChoice();

    if (type == 1) {
        std::string id;
        std::cout << "ID: "; std::getline(std::cin, id);
        const auto found = sampleCtrl_.findById(id);
        if (found.has_value())
            printSampleList({ *found });
        else
            std::cout << "해당 ID의 시료가 없습니다.\n";
    } else if (type == 2) {
        std::string keyword;
        std::cout << "이름 검색어: "; std::getline(std::cin, keyword);
        printSampleList(sampleCtrl_.searchByName(keyword));
    }
}

void MainView::handleOrderMenu() {
    while (true) {
        std::cout << "\n----- 주문 관리 -----\n";
        std::cout << "  1. 주문 접수\n";
        std::cout << "  2. 접수 대기 목록\n";
        std::cout << "  3. 주문 승인\n";
        std::cout << "  4. 주문 거절\n";
        std::cout << "  0. 뒤로\n";
        std::cout << "선택: ";
        const int choice = readMenuChoice();

        switch (choice) {
            case 0: return;
            case 1: {
                std::string sampleId, customerName;
                int quantity;
                std::cout << "시료 ID: "; std::getline(std::cin, sampleId);
                std::cout << "고객명: "; std::getline(std::cin, customerName);
                std::cout << "수량: "; std::cin >> quantity;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (orderCtrl_.createOrder(sampleId, customerName, quantity))
                    std::cout << "주문 접수 완료.\n";
                else
                    std::cout << "주문 실패 (미등록 시료 ID).\n";
                break;
            }
            case 2:
                printOrderList(orderCtrl_.getPendingOrders());
                break;
            case 3: {
                std::string orderId;
                std::cout << "주문 ID: "; std::getline(std::cin, orderId);
                if (orderCtrl_.approveOrder(orderId))
                    std::cout << "주문 승인 완료.\n";
                else
                    std::cout << "승인 실패 (주문 없음).\n";
                break;
            }
            case 4: {
                std::string orderId;
                std::cout << "주문 ID: "; std::getline(std::cin, orderId);
                if (orderCtrl_.rejectOrder(orderId))
                    std::cout << "주문 거절 완료.\n";
                else
                    std::cout << "거절 실패 (주문 없음).\n";
                break;
            }
            default: std::cout << "잘못된 입력입니다.\n"; break;
        }
    }
}

static std::string stockStatusLabel(StockStatus s) {
    switch (s) {
        case StockStatus::SURPLUS:   return "여유";
        case StockStatus::SHORTAGE:  return "부족";
        case StockStatus::DEPLETED:  return "고갈";
        default:                     return "?";
    }
}

void MainView::handleMonitorMenu() {
    while (true) {
        std::cout << "\n----- 모니터링 -----\n";
        std::cout << "  1. 상태별 주문 건수\n";
        std::cout << "  2. 시료별 재고 현황\n";
        std::cout << "  0. 뒤로\n";
        std::cout << "선택: ";
        const int choice = readMenuChoice();
        switch (choice) {
            case 0: return;
            case 1: {
                const OrderStats stats = orderCtrl_.getOrderStats();
                std::cout << "\n----- 상태별 주문 건수 -----\n";
                std::cout << "  접수대기(RESERVED) : " << stats.reserved  << "건\n";
                std::cout << "  생산중  (PRODUCING): " << stats.producing << "건\n";
                std::cout << "  승인완료(CONFIRMED): " << stats.confirmed << "건\n";
                std::cout << "  출고완료(RELEASE)  : " << stats.release   << "건\n";
                break;
            }
            case 2:
                printStockStatusList(sampleCtrl_.getStockStatus(orderCtrl_.getActiveQtyBySample()));
                break;
            default: std::cout << "잘못된 입력입니다.\n"; break;
        }
    }
}

void MainView::printSampleList(const std::vector<Sample>& samples) {
    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }
    std::cout << "\n"
              << std::left
              << std::setw(10) << "ID"
              << std::setw(16) << "이름"
              << std::setw(14) << "생산시간(h)"
              << std::setw(8)  << "수율"
              << std::setw(8)  << "재고" << "\n";
    std::cout << std::string(56, '-') << "\n";
    for (const auto& s : samples) {
        std::cout << std::setw(10) << s.id
                  << std::setw(16) << s.name
                  << std::setw(14) << s.avgProductionTime
                  << std::setw(8)  << s.yield
                  << std::setw(8)  << s.stock << "\n";
    }
}

void MainView::printStockStatusList(const std::vector<SampleStockInfo>& infos) {
    if (infos.empty()) { std::cout << "등록된 시료가 없습니다.\n"; return; }
    std::cout << "\n"
              << std::left
              << std::setw(10) << "ID"
              << std::setw(16) << "이름"
              << std::setw(8)  << "재고"
              << std::setw(8)  << "상태" << "\n";
    std::cout << std::string(42, '-') << "\n";
    for (const auto& info : infos) {
        std::cout << std::setw(10) << info.sample.id
                  << std::setw(16) << info.sample.name
                  << std::setw(8)  << info.sample.stock
                  << std::setw(8)  << stockStatusLabel(info.stockStatus) << "\n";
    }
}

void MainView::handleProductionMenu() {
    while (true) {
        std::cout << "\n----- 생산라인 -----\n";
        std::cout << "  1. 현재 생산 현황\n";
        std::cout << "  2. 생산 대기 큐\n";
        std::cout << "  0. 뒤로\n";
        std::cout << "선택: ";
        const int choice = readMenuChoice();
        switch (choice) {
            case 0: return;
            case 1: {
                const auto info = orderCtrl_.getCurrentProduction();
                if (info.has_value())
                    printProductionInfo(*info);
                else
                    std::cout << "현재 생산 중인 주문이 없습니다.\n";
                break;
            }
            case 2:
                printProductionQueue(orderCtrl_.getProductionQueue());
                break;
            default: std::cout << "잘못된 입력입니다.\n"; break;
        }
    }
}

void MainView::printProductionInfo(const ProductionInfo& info) {
    std::cout << "\n----- 현재 생산 현황 -----\n";
    std::cout << "  주문 ID  : " << info.order.id           << "\n";
    std::cout << "  시료     : " << info.sample.name        << " (" << info.sample.id << ")\n";
    std::cout << "  고객     : " << info.order.customerName << "\n";
    std::cout << "  생산 시작: " << info.order.productionStartedAt << "\n";
    std::cout << "  생산량   : " << info.order.producedQuantity
              << " / " << info.order.targetProductionQuantity << "\n";
    std::cout << "  예상 총시간: " << info.totalProductionHours << "h\n";
}

void MainView::printProductionQueue(const std::vector<ProductionInfo>& queue) {
    if (queue.empty()) { std::cout << "생산 대기 중인 주문이 없습니다.\n"; return; }
    std::cout << "\n"
              << std::left
              << std::setw(10) << "주문ID"
              << std::setw(10) << "시료ID"
              << std::setw(8)  << "생산량"
              << std::setw(8)  << "목표량"
              << std::setw(12) << "총시간(h)"
              << "생산시작" << "\n";
    std::cout << std::string(58, '-') << "\n";
    for (const auto& info : queue) {
        std::cout << std::setw(10) << info.order.id
                  << std::setw(10) << info.sample.id
                  << std::setw(8)  << info.order.producedQuantity
                  << std::setw(8)  << info.order.targetProductionQuantity
                  << std::setw(12) << info.totalProductionHours
                  << info.order.productionStartedAt << "\n";
    }
}

void MainView::printOrderList(const std::vector<Order>& orders) {
    if (orders.empty()) {
        std::cout << "주문이 없습니다.\n";
        return;
    }
    std::cout << "\n"
              << std::left
              << std::setw(22) << "주문 ID"
              << std::setw(10) << "시료 ID"
              << std::setw(14) << "고객명"
              << std::setw(8)  << "수량"
              << std::setw(10) << "상태" << "\n";
    std::cout << std::string(64, '-') << "\n";
    for (const auto& o : orders) {
        std::cout << std::setw(22) << o.id
                  << std::setw(10) << o.sampleId
                  << std::setw(14) << o.customerName
                  << std::setw(8)  << o.quantity
                  << std::setw(10) << statusLabel(o.status) << "\n";
    }
}
