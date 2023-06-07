#pragma once

#include <filesystem>
#include "../business_logic/transport_catalogue.h"
#include "io_requests_base.h"
#include "../external/json.h"
#include "../external/json_builder.h"

namespace TransportGuide::IoRequests {

class JsonReader final : public IoBase, public RenderBase {
public:
    JsonReader(renderer::MapRenderer& map_renderer, TransportGuide::BusinessLogic::TransportCatalogue& catalogue,
            std::istream& input_stream, std::ostream& output_stream);
    void PreloadDocument() override;
    void LoadData() override;
    void SendAnswer() override;
    [[nodiscard]] std::filesystem::path GetOutputFilePath() const;
    [[nodiscard]] std::filesystem::path GetInputFilePath() const;


private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    json::Document document_ = json::Document(json::Node());
    

private:
    void AddStopByNode(const json::Node& node_ptr);
    void AddBusByNode(const json::Node& node_ptr);
    Domain::RenderSettings GetRenderSettings(const json::Node& node);
    Domain::RoutingSettings GetRoutingSettings(const json::Node& node_ptr);
    json::Node GetStopRequestNode(const json::Node& node);
    json::Node GetBusRequestNode(const json::Node& node);
    json::Node GetMapRequestNode(const json::Node& node);
    json::Node GetRouteRequestNode(const json::Node& node);
};

}