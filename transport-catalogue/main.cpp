#include "domain/geo.h"
#include "infrastructure/stream_reader.h"
#include "business_logic/transport_catalogue.h"
#include "tests/tests.h"
#include "infrastructure/json_reader.h"


int main() {
    TransportGuide::Test::AllTests();
/*
    
    TransportGuide::BusinessLogic::TransportCatalogue transport_catalogue{};
    TransportGuide::IoRequests::JsonReader json_reader(transport_catalogue, std::cin, std::cout);
    TransportGuide::IoRequests::IoBase& input_reader = json_reader;
    TransportGuide::IoRequests::IRenderSettings& render_settings = json_reader;
    TransportGuide::renderer::MapRenderer map_renderer(transport_catalogue, std::cout);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    //input_reader.SendAnswer();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
 */
    return 0;
}