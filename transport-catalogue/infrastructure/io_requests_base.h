#pragma once
#include <optional>
#include "../business_logic/transport_catalogue.h"
#include "../business_logic/transport_router.h"
#include "../infrastructure/map_renderer.h"

namespace TransportGuide::IoRequests {

struct IoBase {
public:
    virtual void PreloadDocument() = 0;
    virtual void LoadData() = 0;
    virtual void SendAnswer() = 0;
    
protected:
    explicit IoBase(BusinessLogic::TransportCatalogue& catalogue);
    virtual ~IoBase() = default;
    BusinessLogic::TransportCatalogue& catalogue_;
};

struct RenderBase {
protected:
    explicit RenderBase(renderer::MapRenderer& map_renderer);
    virtual ~RenderBase() = default;
    virtual std::string Render();

protected:
    renderer::MapRenderer& map_renderer_;
};

struct SerializerBase {
public:
    virtual void Serialize(std::ostream& output) = 0;
    virtual void Deserialize(std::istream& input) = 0;
protected:
    explicit SerializerBase(BusinessLogic::SerializerTransportCatalogue& serializer_transport_catalogue);
    virtual ~SerializerBase() = default;
    BusinessLogic::SerializerTransportCatalogue serializer_catalogue_;
};

}