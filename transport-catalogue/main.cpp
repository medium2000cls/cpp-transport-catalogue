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

// Ремарка для ревьюера

// В задании было сказано, сериализовать данные графа и роутера
// Для теста я запустил свое решение, в котором уже была реализованна функция сериализации/десериализации настроек
// маршрутизации, и на удивление тест я прошел.
// Для того, что бы полностью выполнить задание я написал решение которое сериализовало все данные.
// И на удивление тест я не прошел. Хотя очевидно, что десириализация выполнялась в десятки раз быстрее.
// Я написал тест-фреймворк:
// 1) Сериализация всего (Каталог, граф, роутер) - вес 50мб, условное время конструирования 44.2с, сериализации 7.5с, дессириализации 4.4с
// 2) Сериализация части (каталог, граф, вспомогательные поля) - вес 36.2мб, условное время конструирования 43.3с, сериализации 4.5с, дессириализации 44.3с
// 3) Сериализация части (каталог, настройки маршрутов) - вес 1.4мб, условное время конструирования 44.5с, сериализации 0.07с, дессириализации 42.5с
// 4) Сериализация части (каталог, настройки маршрутов, роутер) - вес 15.5мб, условное время конструирования 43.1с, сериализации 3.2с, дессириализации 2.5с
// Вариант 3, я использовал в самом начале. Вариант 1 как раз не прошел тест в тренажере. Вариант 2 по сути бесполезный,
// так как отличия от второго варианта только в худшую сторону. Вариант 4, как мне кажется оптимальный по производительности,
// но вызывает сомнение то что мы пропускаем в сериализации промежуточное звено (граф) на основе которого и строится роутер,
// и впоследствии его использует, что бы находить следующую связанную точку.
// Я бы в работе использовал вариант 1, или 3. Но они противоречат тестам в тренажере и заданию.
// Вопрос, правильно ли убирать определенные этапы сериализации из середины бизнес-логики, сериализуя только дорогие в части вычислений компоненты?
// Варианты можно погонять раскоментировав/закоментировав блоки в serialization.cpp

int main(int argc, char* argv[]) {
    
    TransportGuide::Test::AllTests();
    
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    //Бизнес-логика
    TransportGuide::BusinessLogic::TransportCatalogue transport_catalogue{};

    //Ввод-вывод
    TransportGuide::renderer::MapRenderer map_renderer(transport_catalogue);
    TransportGuide::IoRequests::JsonReader json_reader(map_renderer, transport_catalogue, std::cin, std::cout);

    //Сериализация
    TransportGuide::IoRequests::ProtoSerialization proto_serializer(transport_catalogue, map_renderer);

    //Приведение к базовым классам
    TransportGuide::IoRequests::IoBase& input_reader = json_reader;
    TransportGuide::IoRequests::ISerializer& serializer = proto_serializer;

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