#pragma once

#include <iostream>
#include "transport_catalogue.h"

namespace TransportGuide::IoRequests {

/*Базовый класс предоставляющий интерфейс для получения информации*/
class BaseStatReader {
protected:
    
    TransportGuide::Core::TransportCatalogue& catalogue_;
    explicit BaseStatReader(TransportGuide::Core::TransportCatalogue& catalogue);
    
public:
    
    virtual ~BaseStatReader() = default;
    virtual void SendAnswer() = 0;
};

class StreamStatReader final : public BaseStatReader {
    
    std::istream& input_stream_;
    std::ostream& output_stream_;

public:
    
    explicit StreamStatReader(Core::TransportCatalogue& catalogue, std::istream& input_stream, std::ostream& output_stream);
    void SendAnswer() override;
};
}
