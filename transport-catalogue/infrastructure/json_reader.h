#pragma once

#include "../business_logic/transport_catalogue.h"
#include "io_requests_base.h"
#include "../external/json.h"

namespace TransportGuide::IoRequests {

class JsonReader final : public IoBase, public IRenderSettings{
public:
    JsonReader(BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream, std::ostream& output_stream);
    void PreloadDocument() override;
    void LoadData() override;
    void SendAnswer() override;
    renderer::RenderSettings GetRenderSettings() override;


private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    json::Document document_ = json::Document(json::Node());
    void AddStopByNode(const json::Node* node_ptr);
    void AddBusByNode(const json::Node* node_ptr);
    json::Node GetStopRequestNode(const json::Node& node);
    json::Node GetBusRequestNode(const json::Node& node);
};

}