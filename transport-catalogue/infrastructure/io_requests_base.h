#pragma once

#include "../business_logic/transport_catalogue.h"
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
public:
    virtual renderer::RenderSettings GetRenderSettings() = 0;
protected:
    std::string Render();
    explicit RenderBase(renderer::MapRenderer& map_renderer);
    virtual ~RenderBase() = default;
    renderer::MapRenderer& map_renderer_;
};

}