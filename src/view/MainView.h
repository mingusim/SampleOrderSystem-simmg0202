#pragma once
#include "controller/SampleController.h"
#include "controller/OrderController.h"

class MainView {
public:
    explicit MainView(SampleController& sampleController, OrderController& orderController);
    void run();

private:
    SampleController& sampleCtrl_;
    OrderController&  orderCtrl_;

    static int readMenuChoice();
    void showMainMenu();
    void handleSampleMenu();
    void handleSearchMenu();
    void handleOrderMenu();
    void handleMonitorMenu();
    void handleProductionMenu();
    void printSampleList(const std::vector<Sample>& samples);
    void printOrderList(const std::vector<Order>& orders);
    void printStockStatusList(const std::vector<SampleStockInfo>& infos);
    void printProductionInfo(const ProductionInfo& info);
    void printProductionQueue(const std::vector<ProductionInfo>& queue);
};
