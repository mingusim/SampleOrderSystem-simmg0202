#include "controller/OrderController.h"

OrderController::OrderController(ISampleRepository& sampleRepo, IOrderRepository& orderRepo)
    : sampleRepo_(sampleRepo), orderRepo_(orderRepo) {}
