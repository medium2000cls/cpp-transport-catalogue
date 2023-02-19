#pragma once

#include "transport_catalogue.h"


namespace TransportGuide::IoRequests {

/*Базовый класс предоставляющий интерфейс загрузки данных в каталог*/
class BaseInputReader {
protected:
    
    TransportGuide::Core::TransportCatalogue& catalogue_;
    explicit BaseInputReader(TransportGuide::Core::TransportCatalogue& catalogue);
    
public:
    
    virtual ~BaseInputReader() = default;
    virtual void Load() = 0;
};

class StreamInputReader final : public BaseInputReader {
    
    std::istream& input_stream_;
    void AddStopByDescription(std::string_view str_view);
    void AddBusByDescription(std::string_view str_view);
    
public:
    
    explicit StreamInputReader(Core::TransportCatalogue& catalogue, std::istream& input_stream);
    void Load() override;
};
}