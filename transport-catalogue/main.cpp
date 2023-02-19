#include "geo.h"
#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"
#include "tests.h"


int main() {
    TransportGuide::Test::AllTests();
/*
    TransportGuide::Core::TransportCatalogue transport_catalogue{};
    TransportGuide::IoRequests::StreamInputReader stream_input_reader(transport_catalogue, std::cin);
    TransportGuide::IoRequests::BaseInputReader& input_reader = stream_input_reader;
    
    TransportGuide::IoRequests::StreamStatReader stream_stat_reader (transport_catalogue, std::cin, std::cout);
    TransportGuide::IoRequests::BaseStatReader& stat_reader = stream_stat_reader;
    
    input_reader.Load();
    stat_reader.SendAnswer();
*/
    
    return 0;
}