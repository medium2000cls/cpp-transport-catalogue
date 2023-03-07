#pragma once

#include <iostream>
#include "../business_logic/transport_catalogue.h"
#include "io_requests_base.h"


/*Базовый класс предоставляющий интерфейс для получения информации*/
namespace TransportGuide::IoRequests {

class StreamReader final : public IoRequestsBase {
public:
    StreamReader(BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream, std::ostream& output_stream);
    
    void LoadData() override;
    void SendAnswer() override;
    
private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    void AddStopByDescription(std::string_view str_view);
    void AddBusByDescription(std::string_view str_view);
};
}