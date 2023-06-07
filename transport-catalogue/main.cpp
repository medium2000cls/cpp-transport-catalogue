#include <iostream>
#include <fstream>
#include "domain/geo.h"
#include "infrastructure/stream_reader.h"
#include "business_logic/transport_catalogue.h"
#include "tests/tests.h"
#include "infrastructure/json_reader.h"
#include "infrastructure/serialization.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}


int main(int argc, char* argv[]) {
    
    TransportGuide::Test::AllTests();
    
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    //Бизнес-логика
    TransportGuide::BusinessLogic::TransportCatalogue transport_catalogue{};
    TransportGuide::BusinessLogic::SerializerTransportCatalogue serializer_transport_catalogue (transport_catalogue);

    //Ввод-вывод
    TransportGuide::renderer::MapRenderer map_renderer(transport_catalogue);
    TransportGuide::IoRequests::JsonReader json_reader(map_renderer, transport_catalogue, std::cin, std::cout);

    //Сериализация
    TransportGuide::IoRequests::ProtoSerialization proto_serializer(serializer_transport_catalogue, map_renderer);

    //Приведение к базовым классам
    TransportGuide::IoRequests::IoBase& input_reader = json_reader;
    TransportGuide::IoRequests::SerializerBase& serializer = proto_serializer;

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        input_reader.PreloadDocument();
        input_reader.LoadData();
        std::ofstream output_file(json_reader.GetOutputFilePath(), std::ios::binary);
        serializer.Serialize(output_file);

    } else if (mode == "process_requests"sv) {
        input_reader.PreloadDocument();
        std::ifstream input_file(json_reader.GetInputFilePath(), std::ios::binary);
        serializer.Deserialize(input_file);
        input_reader.SendAnswer();

    } else {
        PrintUsage();
        return 1;
    }
}