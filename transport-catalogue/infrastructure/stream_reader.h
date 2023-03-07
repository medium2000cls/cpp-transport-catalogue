#pragma once
#include <sstream>
#include <iostream>
#include "../business_logic/transport_catalogue.h"
#include "io_requests_base.h"


/*Базовый класс предоставляющий интерфейс для получения информации*/
namespace TransportGuide::IoRequests {

class StreamReader final : public IoBase {
public:
    StreamReader(BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream, std::ostream& output_stream);
    void PreloadDocument() override;
    void LoadData() override;
    void SendAnswer() override;
   
private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    std::string document_;
    std::istringstream input_string_stream_;
    
    void AddStopByDescription(std::string_view str_view);
    void AddBusByDescription(std::string_view str_view);
};
}