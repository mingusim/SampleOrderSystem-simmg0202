#include "view/MainView.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

MainView::MainView(SampleController& sampleController)
    : sampleCtrl_(sampleController) {}

int MainView::readMenuChoice() {
    int choice = 0;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

void MainView::run() {
    while (true) {
        showMainMenu();
        switch (readMenuChoice()) {
            case 1: handleSampleMenu(); break;
            case 0: return;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void MainView::showMainMenu() {
    std::cout << "\n========== 시료 생산주문관리 시스템 ==========\n";
    std::cout << "  1. 시료 관리\n";
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
