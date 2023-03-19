#include <sstream>
#include "io_requests_base.h"


TransportGuide::IoRequests::IoBase::IoBase(TransportGuide::BusinessLogic::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

TransportGuide::IoRequests::RenderBase::RenderBase(TransportGuide::renderer::MapRenderer& map_renderer) : map_renderer_(
        map_renderer) {}

std::string TransportGuide::IoRequests::RenderBase::Render() {
    std::ostringstream out;
    map_renderer_.CreateDocument(GetRenderSettings());
    map_renderer_.Render(out);
    return out.str();
}
