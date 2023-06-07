#pragma once

#include "../business_logic/transport_catalogue.h"
#include "json_reader.h"

namespace TransportGuide::IoRequests {

class ProtoSerialization : public TransportGuide::IoRequests::SerializerBase {
public:
    explicit ProtoSerialization(BusinessLogic::SerializerTransportCatalogue& serializer_transport_catalogue, renderer::MapRenderer& map_renderer);
    
    void Serialize(std::ostream& output) override;
    void Deserialize(std::istream& input) override;
    
private:
    renderer::MapRenderer& map_renderer_;
};

}