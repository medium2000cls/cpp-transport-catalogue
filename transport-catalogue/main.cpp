#include "domain/geo.h"
#include "infrastructure/stream_reader.h"
#include "business_logic/transport_catalogue.h"
#include "tests/tests.h"
#include "infrastructure/json_reader.h"


int main() {
//    TransportGuide::Test::AllTests();
    
    TransportGuide::BusinessLogic::TransportCatalogue transport_catalogue{};
    TransportGuide::IoRequests::JsonReader json_reader(transport_catalogue, std::cin, std::cout);
    TransportGuide::IoRequests::IoRequestsBase& io_requests_base = json_reader;

    io_requests_base.LoadData();
    io_requests_base.SendAnswer();
    
    return 0;
}