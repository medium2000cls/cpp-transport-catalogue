#include <random>
#include <sstream>
#include <fstream>
#include "tests.h"
#include "input_reader.h"
#include "stat_reader.h"

namespace TransportGuide::Test {
using namespace std::literals;

constexpr double ACCURACY_COMPARISON = 1e-6;

//region assist definition
void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
        unsigned int line, const std::string& hint) {
    using namespace std;
    
    if (!value) {
//        cerr << file << "("s << line << "): "s << func << ": "s;
//        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint << endl;
        }
//        cerr << endl;
        throw invalid_argument("failed test");
    }
}

void RunTest(const std::string& func_name) {
    std::cerr << "   OK      > " << func_name << std::endl;
}
void AbortTest(const std::string& func_name) {
    std::cerr << "   FAILED  > " << func_name << std::endl;
}

std::string GetTextFromStream(std::istream& input) {
    std::string str;
    std::string file_contents;
    while (std::getline(input, str))
    {
        file_contents += str;
        file_contents.push_back('\n');
    }
    return file_contents;
}


#ifdef __unix__
#define LINUX
#else
#define WINDOWS
#endif

#ifdef WINDOWS
std::string getexepath()
{
    return ".";
}
#else
#include <limits.h>
#include <unistd.h>
std::string getexepath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    std::string path = std::string(result, (count > 0) ? count : 0);
    path = path.substr(0, path.find_last_of('/'));
    return path;
}
#endif



//endregion

//region generators
std::string GenerateWord(std::mt19937& generator, int max_length) {
    const int length = std::uniform_int_distribution(1, max_length)(generator);
    std::string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(std::uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

std::deque<Core::Stop> StopGenerator(size_t count) {
    std::mt19937 generator;
    std::deque<Core::Stop> result;
    for (size_t i = 0; i < count; ++i) {
        const std::string& name = GenerateWord(generator, 50);
        double B = std::uniform_real_distribution(30., 50.)(generator);
        double L = std::uniform_real_distribution(30., 50.)(generator);
        result.push_back(Core::Stop (name, B, L));
    }
    return result;
}

std::deque<Core::Bus> BusGenerator(std::deque<Core::Stop>& stops, size_t count) {
    std::mt19937 generator;
    int stops_bound = stops.size() - 1;
    std::deque<Core::Bus> result;
    for (size_t i = 0; i < count; ++i) {
        std::string name = GenerateWord(generator, 12);
        std::vector<const Core::Stop*> route;
        const size_t length = std::uniform_int_distribution(3, 100)(generator);
        for (size_t j = 0; j < length; ++j) {
            size_t index = std::uniform_int_distribution(0, stops_bound)(generator);
            route.push_back(&stops[index]);
        }
        route.push_back(route.front());
        
        Core::Bus bus (name, route, 100, 200);
        result.push_back(std::move(bus));
    }
    return result;

}

std::deque<Core::TrackSection> TrackSectionCatalogGenerator(std::deque<Core::Stop>& stops, size_t count) {
    std::mt19937 generator;
    int stops_bound = stops.size() - 1;
    std::deque<Core::TrackSection> result;
    for (size_t i = 0; i < count; ++i) {
        const size_t a = std::uniform_int_distribution(0, stops_bound / 2)(generator);
        const size_t b = std::uniform_int_distribution(stops_bound / 2 + 1, stops_bound)(generator);
        Core::TrackSection track_section{&stops[a], &stops[b]};
        result.push_back(std::move(track_section));
    }
    return result;
}
//endregion

//region TransportCatalogue СhildClass definition
std::deque<Core::Stop>& TransportCatalogue::GetStops() {
    return stop_catalog_;
}

std::deque<Core::Bus>& TransportCatalogue::GetBuses() {
    return bus_catalog_;
}

void TransportCatalogue::AddRealDistanceToCatalog(Core::TrackSection track_section, double distance) {
    return Core::TransportCatalogue::AddRealDistanceToCatalog(track_section, distance);
}

double TransportCatalogue::GetBusRealLength(const std::vector<const Core::Stop*>& route) {
    return Core::TransportCatalogue::GetBusRealLength(route);
}
//endregion
void IntegrationTests::TestCase_5_PlusRealRoutersAndCurveInBusInformation() {
    std::ifstream file_input_stream(getexepath() + "/test_case/tsC_case1_input.txt");
    std::ifstream file_output_stream_1(getexepath() + "/test_case/tsC_case1_output1.txt");
    std::ifstream file_output_stream_2(getexepath() + "/test_case/tsC_case1_output2.txt");
    
    TransportCatalogue transport_catalogue{};
    IoRequests::StreamInputReader stream_input_reader(transport_catalogue, file_input_stream);
    IoRequests::BaseInputReader& input_reader = stream_input_reader;
    
    std::ostringstream o_string_stream;
    IoRequests::StreamStatReader stream_stat_reader (transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::BaseStatReader& stat_reader = stream_stat_reader;
    
    input_reader.Load();
    stat_reader.SendAnswer();
    
    std::string correct_answer_1 = GetTextFromStream(file_output_stream_1);
    std::string correct_answer_2 = GetTextFromStream(file_output_stream_2);
    
    auto answer = o_string_stream.str();
    ASSERT(answer == correct_answer_1 || answer == correct_answer_2);
}

void TransportCatalogueTests::TrackSectionHasher() {
    size_t max_collision_count = 0;
    size_t count_collision_more_one = 0;
    std::unordered_map<size_t, int> collision;
    std::deque<Core::Stop> stops = StopGenerator(15'000);
    int count_track = 1'000'000;
    std::deque<Core::TrackSection> track_section_catalog = TrackSectionCatalogGenerator(stops, count_track);
    Core::TrackSectionHasher hasher;
    {
        LogDuration x("TrackSectionHasher - time hasher");
        std::for_each(track_section_catalog.begin(), track_section_catalog.end(),
                [&](const Core::TrackSection& track_section) {
                    size_t value = ++collision[hasher(track_section)];
                    max_collision_count = max_collision_count < value ? value : max_collision_count;
                    count_collision_more_one = value > 1 ? count_collision_more_one + 1 : count_collision_more_one;
                });
    }
    double ratio_collision_all = static_cast<double>(count_collision_more_one) / static_cast<double>(count_track);
    ASSERT_HINT(max_collision_count < 4, std::to_string(max_collision_count) + " < 4 max count collision");
    ASSERT_HINT(ratio_collision_all < 0.05, std::to_string(ratio_collision_all) + " < 0.05 => ratio = collision/all track"s);
}

void TransportCatalogueTests::AddBus() {
    TransportCatalogue transport_catalogue{};
    
    std::deque<Core::Bus> null_buses;
    ASSERT(transport_catalogue.GetBuses() == null_buses);
    
    std::deque<Core::Stop> stops = StopGenerator(30);
    std::deque<Core::Bus> buses = BusGenerator(stops, 20);
    for (const Core::Bus& bus : buses) {
        transport_catalogue.InsertBus(bus);
    }
    ASSERT(transport_catalogue.GetBuses() == buses);
}

void TransportCatalogueTests::AddStop() {
    TransportCatalogue transport_catalogue{};
    
    std::deque<Core::Stop> null_stops;
    ASSERT(transport_catalogue.GetStops() == null_stops);
    
    std::deque<Core::Stop> stops = StopGenerator(30);
    for (const Core::Stop& stop : stops) {
        transport_catalogue.InsertStop(stop);
    }
    ASSERT(transport_catalogue.GetStops() == stops);
}

void TransportCatalogueTests::FindBus() {
    TransportCatalogue transport_catalogue{};
    std::deque<Core::Stop> stops = StopGenerator(30);
    std::deque<Core::Bus> buses = BusGenerator(stops, 20);
    auto buses_size = buses.size();
    for (size_t i = 0; i < buses_size / 2; ++i) {
        transport_catalogue.InsertBus(buses[i]);
    }
    for (size_t i = 0; i < buses_size; ++i) {
        const auto& bus_optional = transport_catalogue.FindBus(buses[i].name);
        if (i < buses_size / 2) {
            ASSERT(bus_optional.has_value() && buses[i] == *bus_optional.value());
        }
        else {
            ASSERT(!bus_optional.has_value());
        }
    }
}

void TransportCatalogueTests::FindStop() {
    TransportCatalogue transport_catalogue{};
    std::deque<Core::Stop> stops = StopGenerator(30);
    auto stops_size = stops.size();
    for (size_t i = 0; i < stops_size / 2; ++i) {
        transport_catalogue.InsertStop(stops[i]);
    }
    for (size_t i = 0; i < stops_size; ++i) {
        const auto& stop_optional = transport_catalogue.FindStop(stops[i].name);
        if (i < stops_size / 2) {
            ASSERT(stop_optional.has_value() && stops[i] == *stop_optional.value());
        }
        else {
            ASSERT(!stop_optional.has_value());
        }
    }
}

void TransportCatalogueTests::GetBusInfo() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Core::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Core::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Core::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Core::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Core::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(Core::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 4371.02));
    transport_catalogue.InsertBus(Core::Bus("750", {&stops[0], &stops[1], &stops[2], &stops[1], &stops[0]}, 20939.5, 20939.5));
    auto& buses = transport_catalogue.GetBuses();
    
    TransportGuide::Core::BusInfo bus_info1 {"256", 6, 5, 4371.02, 1};
    TransportGuide::Core::BusInfo bus_info2 {"750", 5, 3, 20939.5, 1};
    
    std::optional<Core::BusInfo> bus_info_256 = transport_catalogue.GetBusInfo(&buses[0]);
    std::optional<Core::BusInfo> bus_info_750 = transport_catalogue.GetBusInfo(&buses[1]);
    
    ASSERT(bus_info_256.has_value() && bus_info_256.value() == bus_info1);
    ASSERT(bus_info_750.has_value() && bus_info_750.value() == bus_info2);
    ASSERT(!transport_catalogue.GetBusInfo(nullptr).has_value());
}

void TransportCatalogueTests::GetBusInfoPlusCurvatureAdded() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Core::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Core::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Core::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Core::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Core::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Core::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Core::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(Core::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(Core::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5, 27400));
    transport_catalogue.InsertBus(Core::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
    auto& buses = transport_catalogue.GetBuses();
    transport_catalogue.AddRealDistanceToCatalog({&stops[0], &stops[1]}, 3900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[1], &stops[2]}, 9900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[1], &stops[1]}, 100);
    transport_catalogue.AddRealDistanceToCatalog({&stops[2], &stops[1]}, 9500);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[8]}, 7500);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[4]}, 1800);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[5]}, 2400);
    transport_catalogue.AddRealDistanceToCatalog({&stops[4], &stops[5]}, 750);
    transport_catalogue.AddRealDistanceToCatalog({&stops[5], &stops[8]}, 5600);
    transport_catalogue.AddRealDistanceToCatalog({&stops[5], &stops[6]}, 900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[6], &stops[7]}, 1300);
    transport_catalogue.AddRealDistanceToCatalog({&stops[7], &stops[3]}, 1200);
    
    TransportGuide::Core::BusInfo bus_info1 {"256", 6, 5, 5950, 1.36124};
    TransportGuide::Core::BusInfo bus_info2 {"750", 7, 3, 27400, 1.30853};
    
    std::optional<Core::BusInfo> bus_info_256 = transport_catalogue.GetBusInfo(&buses[0]);
    std::optional<Core::BusInfo> bus_info_750 = transport_catalogue.GetBusInfo(&buses[1]);
    
    ASSERT(!transport_catalogue.GetBusInfo(nullptr).has_value());
    ASSERT(bus_info_256.has_value() && bus_info_256.value() == bus_info1);
    ASSERT(bus_info_750.has_value() && bus_info_750.value() == bus_info2);
}

void TransportCatalogueTests::GetStopInfo() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Core::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Core::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Core::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Core::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Core::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Core::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Core::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(Core::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(Core::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5, 27400));
    transport_catalogue.InsertBus(Core::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
    auto& buses = transport_catalogue.GetBuses();
    
    TransportGuide::Core::StopInfo stop_info_2 {"Prazhskaya", {}};
    TransportGuide::Core::StopInfo stop_info_3 {"Biryulyovo Zapadnoye", {&buses[0], &buses[2]}};
    
    std::optional<Core::StopInfo> stop_info_samara = transport_catalogue.GetStopInfo(nullptr);
    std::optional<Core::StopInfo> stop_info_prazhskaya = transport_catalogue.GetStopInfo(&stops[9]);
    std::optional<Core::StopInfo> stop_info_birul = transport_catalogue.GetStopInfo(&stops[3]);
    
    ASSERT(!stop_info_samara.has_value());
    ASSERT(stop_info_prazhskaya.has_value() && stop_info_prazhskaya.value() == stop_info_2);
    ASSERT(stop_info_birul.has_value() && stop_info_birul.value() == stop_info_3);
}

void ConsoleInputReaderTests::Load() {
    std::istringstream string_stream("13\n"
                                     "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino\n"
                                     "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n"
                                     "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
                                     "Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka\n"
                                     "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino\n"
                                     "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam\n"
                                     "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam\n"
                                     "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya\n"
                                     "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya\n"
                                     "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye\n"
                                     "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
                                     "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
                                     "Stop Prazhskaya: 55.611678, 37.603831\n"s);
    TransportCatalogue transport_catalogue{};
    IoRequests::StreamInputReader stream_input_reader(transport_catalogue, string_stream);
    IoRequests::BaseInputReader& input_reader = stream_input_reader;
    input_reader.Load();
    std::deque<Core::Bus>& buses = transport_catalogue.GetBuses();
    ASSERT(buses.size() == 3);
    std::deque<Core::Stop>& stops = transport_catalogue.GetStops();
    ASSERT(stops.size() == 10);
    Core::Stop stop("Biryulyovo Tovarnaya", 55.592028, 37.653656);
    ASSERT(transport_catalogue.FindStop("Biryulyovo Tovarnaya").has_value() && *transport_catalogue.FindStop("Biryulyovo Tovarnaya").value() == stop);
    
    Core::Bus bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5, 27400);
    ASSERT((transport_catalogue.FindBus("750").has_value() && *transport_catalogue.FindBus("750").value() == bus));
    ASSERT(transport_catalogue.GetBusRealLength(transport_catalogue.FindBus("750").value()->route) == 27400);
}

void ConsoleStatReaderTests::SendAnswer() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Core::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Core::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Core::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Core::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Core::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Core::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Core::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Core::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(Core::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(Core::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5, 27400));
    transport_catalogue.InsertBus(Core::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
    auto& buses = transport_catalogue.GetBuses();
    transport_catalogue.AddRealDistanceToCatalog({&stops[0], &stops[1]}, 3900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[1], &stops[2]}, 9900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[1], &stops[1]}, 100);
    transport_catalogue.AddRealDistanceToCatalog({&stops[2], &stops[1]}, 9500);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[8]}, 7500);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[4]}, 1800);
    transport_catalogue.AddRealDistanceToCatalog({&stops[3], &stops[5]}, 2400);
    transport_catalogue.AddRealDistanceToCatalog({&stops[4], &stops[5]}, 750);
    transport_catalogue.AddRealDistanceToCatalog({&stops[5], &stops[8]}, 5600);
    transport_catalogue.AddRealDistanceToCatalog({&stops[5], &stops[6]}, 900);
    transport_catalogue.AddRealDistanceToCatalog({&stops[6], &stops[7]}, 1300);
    transport_catalogue.AddRealDistanceToCatalog({&stops[7], &stops[3]}, 1200);
    
    std::istringstream i_string_stream("6\n"
                                       "Bus 256\n"
                                       "Bus 750\n"
                                       "Bus 751\n"
                                       "Stop Samara\n"
                                       "Stop Prazhskaya\n"
                                       "Stop Biryulyovo Zapadnoye "s);
    std::ostringstream o_string_stream;
    
    IoRequests::StreamStatReader stream_stat_reader (transport_catalogue, i_string_stream, o_string_stream);
    IoRequests::BaseStatReader& stat_reader = stream_stat_reader;
    stat_reader.SendAnswer();
    std::string correct_answer = "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n"
                                 "Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature\n"
                                 "Bus 751: not found\n"
                                 "Stop Samara: not found\n"
                                 "Stop Prazhskaya: no buses\n"
                                 "Stop Biryulyovo Zapadnoye: buses 256 828\n"s;
    const auto& str = o_string_stream.str();
    ASSERT(str == correct_answer);
}

void AllTests() {
    IntegrationTests integration_tests;
    RUN_TEST(integration_tests.TestCase_5_PlusRealRoutersAndCurveInBusInformation)
    TransportCatalogueTests transport_catalogue_tests;
    RUN_TEST(transport_catalogue_tests.TrackSectionHasher)
    RUN_TEST(transport_catalogue_tests.AddBus)
    RUN_TEST(transport_catalogue_tests.AddStop)
    RUN_TEST(transport_catalogue_tests.FindBus)
    RUN_TEST(transport_catalogue_tests.FindStop)
    RUN_TEST(transport_catalogue_tests.GetBusInfo)
    RUN_TEST(transport_catalogue_tests.GetBusInfoPlusCurvatureAdded)
    RUN_TEST(transport_catalogue_tests.GetStopInfo)
    ConsoleInputReaderTests console_input_reader_tests;
    RUN_TEST(console_input_reader_tests.Load)
    ConsoleStatReaderTests console_stat_reader_tests;
    RUN_TEST(console_stat_reader_tests.SendAnswer)
}
}