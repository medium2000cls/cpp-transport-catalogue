#pragma once

#include "../business_logic/transport_catalogue.h"

namespace TransportGuide::IoRequests {

struct IoRequestsBase {
public:
    virtual void LoadData() = 0;
    virtual void SendAnswer() = 0;
    
protected:
    explicit IoRequestsBase(BusinessLogic::TransportCatalogue& catalogue);
    virtual ~IoRequestsBase() = default;
    BusinessLogic::TransportCatalogue& catalogue_;
};

}