#pragma once

#include "../business_logic/transport_catalogue.h"
#include "json_reader.h"

namespace TransportGuide::IoRequests {

class ProtoSerialization : public TransportGuide::IoRequests::ISerializer {
public:
    explicit ProtoSerialization(BusinessLogic::TransportCatalogue& transport_catalogue, renderer::MapRenderer& map_renderer);
    
    void Serialize(std::ostream& output) override;
    void Deserialize(std::istream& input) override;
    
private:
    BusinessLogic::TransportCatalogue& transport_catalogue_;
    renderer::MapRenderer& map_renderer_;
};

}