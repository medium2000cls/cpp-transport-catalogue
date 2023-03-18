#include <random>
#include <sstream>
#include <fstream>
#include "tests.h"
#include "../infrastructure/stream_reader.h"
#include "../domain/domain.h"
#include "../infrastructure/json_reader.h"

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
void AbortTest(const std::string& func_name, std::string exception_text) {
    std::cerr << "   FAILED  > " << func_name << std::endl;
    if (!exception_text.empty()) {
        std::cerr << exception_text << std::endl;
    }
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

std::deque<Domain::Stop> StopGenerator(size_t count) {
    std::mt19937 generator;
    std::deque<Domain::Stop> result;
    for (size_t i = 0; i < count; ++i) {
        const std::string& name = GenerateWord(generator, 50);
        double B = std::uniform_real_distribution(30., 50.)(generator);
        double L = std::uniform_real_distribution(30., 50.)(generator);
        result.push_back(Domain::Stop(name, B, L));
    }
    return result;
}

std::deque<Domain::Bus> BusGenerator(std::deque<Domain::Stop>& stops, size_t count) {
    std::mt19937 generator;
    int stops_bound = stops.size() - 1;
    std::deque<Domain::Bus> result;
    for (size_t i = 0; i < count; ++i) {
        std::string name = GenerateWord(generator, 12);
        std::vector<const Domain::Stop*> route;
        const size_t length = std::uniform_int_distribution(3, 100)(generator);
        for (size_t j = 0; j < length; ++j) {
            size_t index = std::uniform_int_distribution(0, stops_bound)(generator);
            route.push_back(&stops[index]);
        }
        route.push_back(route.front());
        
        Domain::Bus bus(name, route, 100, 200);
        result.push_back(std::move(bus));
    }
    return result;

}

std::deque<Domain::TrackSection> TrackSectionCatalogGenerator(std::deque<Domain::Stop>& stops, size_t count) {
    std::mt19937 generator;
    int stops_bound = stops.size() - 1;
    std::deque<Domain::TrackSection> result;
    for (size_t i = 0; i < count; ++i) {
        const size_t a = std::uniform_int_distribution(0, stops_bound / 2)(generator);
        const size_t b = std::uniform_int_distribution(stops_bound / 2 + 1, stops_bound)(generator);
        Domain::TrackSection track_section{&stops[a], &stops[b]};
        result.push_back(std::move(track_section));
    }
    return result;
}
//endregion

//region TransportCatalogue СhildClass definition

std::deque<Domain::Stop>& TransportCatalogue::GetStops() {
    return stop_catalog_;
}

std::deque<Domain::Bus>& TransportCatalogue::GetBuses() {
    return bus_catalog_;
}

void TransportCatalogue::AddRealDistanceToCatalog(Domain::TrackSection track_section, double distance) {
    return BusinessLogic::TransportCatalogue::AddRealDistanceToCatalog(track_section, distance);
}

double TransportCatalogue::GetBusRealLength(const std::vector<const Domain::Stop*>& route) {
    return BusinessLogic::TransportCatalogue::GetBusRealLength(route);
}

//endregion

void IntegrationTests::TestCase_5_PlusRealRoutersAndCurveInBusInformation() {
    std::ifstream file_input_stream(getexepath() + "/test_case/tsC_case1_input.txt");
    std::ifstream file_output_stream_1(getexepath() + "/test_case/tsC_case1_output1.txt");
    std::ifstream file_output_stream_2(getexepath() + "/test_case/tsC_case1_output2.txt");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::StreamReader stream_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = stream_reader;
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    input_reader.SendAnswer();
    
    std::string correct_answer_1 = GetTextFromStream(file_output_stream_1);
    std::string correct_answer_2 = GetTextFromStream(file_output_stream_2);
    
    auto answer = o_string_stream.str();
    ASSERT(answer == correct_answer_1 || answer == correct_answer_2);
}

void IntegrationTests::TestCase_6_JsonReader() {
    std::istringstream i_string_stream("{\n" "  \"base_requests\": [\n" "    {\n" "      \"type\": \"Bus\",\n"
                                       "      \"name\": \"114\",\n" "      \"stops\": [\"Морской вокзал\", \"Ривьерский мост\"],\n"
                                       "      \"is_roundtrip\": false\n" "    },\n" "    {\n" "      \"type\": \"Stop\",\n"
                                       "      \"name\": \"Ривьерский мост\",\n" "      \"latitude\": 43.587795,\n"
                                       "      \"longitude\": 39.716901,\n" "      \"road_distances\": {\"Морской вокзал\": 850}\n"
                                       "    },\n" "    {\n" "      \"type\": \"Stop\",\n" "      \"name\": \"Морской вокзал\",\n"
                                       "      \"latitude\": 43.581969,\n" "      \"longitude\": 39.719848,\n"
                                       "      \"road_distances\": {\"Ривьерский мост\": 850}\n" "    }\n" "  ],\n"
                                       "  \"stat_requests\": [\n" "    { \"id\": 1, \"type\": \"Stop\", \"name\": \"Ривьерский мост\" },\n"
                                       "    { \"id\": 2, \"type\": \"Bus\", \"name\": \"114\" }\n" "  ]\n" "} "s);
    
    std::ostringstream o_string_stream;
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, i_string_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    input_reader.SendAnswer();
    
    std::string correct_answer_1 = "[\n" "    {\n" "        \"buses\": [\n" "            \"114\"\n" "        ],\n"
                                   "        \"request_id\": 1\n" "    },\n" "    {\n" "        \"curvature\": 1.23199,\n"
                                   "        \"request_id\": 2,\n" "        \"route_length\": 1700,\n" "        \"stop_count\": 3,\n"
                                   "        \"unique_stop_count\": 2\n" "    }\n" "] "s;
    
    auto answer = o_string_stream.str();
    
    std::istringstream answer_stream = std::istringstream(answer);
    json::Document answer_doc = json::Load(answer_stream);
    
    std::istringstream correct_stream = std::istringstream(correct_answer_1);
    json::Document correct_doc = json::Load(correct_stream);
    
    ASSERT(answer_doc == correct_doc);
    
}

void IntegrationTests::TestCase_7_MapRender() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_integration_input.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    std::string correct_answer_1 = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                                   "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"
                                   "  <polyline points=\"99.2283,329.5 50,232.18 99.2283,329.5\" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n"
                                   "  <polyline points=\"550,190.051 279.22,50 333.61,269.08 550,190.051\" fill=\"none\" stroke=\"rgb(255,160,0)\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n"
                                   "  <text fill=\"green\" x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"232.18\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n"
                                   "  <text fill=\"green\" x=\"50\" y=\"232.18\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"550\" y=\"190.051\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n"
                                   "  <text fill=\"rgb(255,160,0)\" x=\"550\" y=\"190.051\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n"
                                   "  <circle cx=\"279.22\" cy=\"50\" r=\"5\" fill=\"white\"/>\n"
                                   "  <circle cx=\"99.2283\" cy=\"329.5\" r=\"5\" fill=\"white\"/>\n"
                                   "  <circle cx=\"50\" cy=\"232.18\" r=\"5\" fill=\"white\"/>\n"
                                   "  <circle cx=\"333.61\" cy=\"269.08\" r=\"5\" fill=\"white\"/>\n"
                                   "  <circle cx=\"550\" cy=\"190.051\" r=\"5\" fill=\"white\"/>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"279.22\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Elektroseti</text>\n"
                                   "  <text fill=\"black\" x=\"279.22\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Elektroseti</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Morskoy vokzal</text>\n"
                                   "  <text fill=\"black\" x=\"99.2283\" y=\"329.5\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Morskoy vokzal</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"232.18\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Rivierskiy most</text>\n"
                                   "  <text fill=\"black\" x=\"50\" y=\"232.18\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Rivierskiy most</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"333.61\" y=\"269.08\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ulitsa Dokuchaeva</text>\n"
                                   "  <text fill=\"black\" x=\"333.61\" y=\"269.08\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ulitsa Dokuchaeva</text>\n"
                                   "  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"550\" y=\"190.051\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ulitsa Lizy Chaikinoi</text>\n"
                                   "  <text fill=\"black\" x=\"550\" y=\"190.051\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\">Ulitsa Lizy Chaikinoi</text>\n"
                                   "</svg>"s;
    auto answer = o_string_stream.str();
/*
    std::cout << correct_answer_1 << std::endl;
    std::cout << std::endl;
    std::cout << answer << std::endl;
*/
    
    ASSERT(answer == correct_answer_1);
    
}

void TransportCatalogueTests::TrackSectionHasher() {
    size_t max_collision_count = 0;
    size_t count_collision_more_one = 0;
    std::unordered_map<size_t, int> collision;
    std::deque<Domain::Stop> stops = StopGenerator(15'000);
    int count_track = 1'000'000;
    std::deque<Domain::TrackSection> track_section_catalog = TrackSectionCatalogGenerator(stops, count_track);
    Domain::TrackSectionHasher hasher;
    {
        LogDuration x("TrackSectionHasher - time hasher");
        std::for_each(track_section_catalog.begin(), track_section_catalog.end(),
                [&](const Domain::TrackSection& track_section) {
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
    
    std::deque<Domain::Bus> null_buses;
    ASSERT(transport_catalogue.GetBuses() == null_buses);
    
    std::deque<Domain::Stop> stops = StopGenerator(30);
    std::deque<Domain::Bus> buses = BusGenerator(stops, 20);
    for (const Domain::Bus& bus : buses) {
        transport_catalogue.InsertBus(bus);
    }
    ASSERT(transport_catalogue.GetBuses() == buses);
}

void TransportCatalogueTests::AddStop() {
    TransportCatalogue transport_catalogue{};
    
    std::deque<Domain::Stop> null_stops;
    ASSERT(transport_catalogue.GetStops() == null_stops);
    
    std::deque<Domain::Stop> stops = StopGenerator(30);
    for (const Domain::Stop& stop : stops) {
        transport_catalogue.InsertStop(stop);
    }
    ASSERT(transport_catalogue.GetStops() == stops);
}

void TransportCatalogueTests::FindBus() {
    TransportCatalogue transport_catalogue{};
    std::deque<Domain::Stop> stops = StopGenerator(30);
    std::deque<Domain::Bus> buses = BusGenerator(stops, 20);
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
    std::deque<Domain::Stop> stops = StopGenerator(30);
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
    transport_catalogue.InsertStop(Domain::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Domain::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Domain::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Domain::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Domain::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(
            Domain::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 4371.02));
    transport_catalogue.InsertBus(
            Domain::Bus("750", {&stops[0], &stops[1], &stops[2], &stops[1], &stops[0]}, 20939.5, 20939.5));
    auto& buses = transport_catalogue.GetBuses();
    
    Domain::BusInfo bus_info1{"256", 6, 5, 4371.02, 1};
    Domain::BusInfo bus_info2{"750", 5, 3, 20939.5, 1};
    
    std::optional<Domain::BusInfo> bus_info_256 = transport_catalogue.GetBusInfo(&buses[0]);
    std::optional<Domain::BusInfo> bus_info_750 = transport_catalogue.GetBusInfo(&buses[1]);
    
    ASSERT(bus_info_256.has_value() && bus_info_256.value() == bus_info1);
    ASSERT(bus_info_750.has_value() && bus_info_750.value() == bus_info2);
    ASSERT(!transport_catalogue.GetBusInfo(nullptr).has_value());
}

void TransportCatalogueTests::GetBusInfoPlusCurvatureAdded() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Domain::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Domain::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Domain::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Domain::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Domain::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Domain::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Domain::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(
            Domain::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(
            Domain::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5,
                    27400));
    transport_catalogue.InsertBus(Domain::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
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
    
    Domain::BusInfo bus_info1{"256", 6, 5, 5950, 1.36124};
    Domain::BusInfo bus_info2{"750", 7, 3, 27400, 1.30853};
    
    std::optional<Domain::BusInfo> bus_info_256 = transport_catalogue.GetBusInfo(&buses[0]);
    std::optional<Domain::BusInfo> bus_info_750 = transport_catalogue.GetBusInfo(&buses[1]);
    
    ASSERT(!transport_catalogue.GetBusInfo(nullptr).has_value());
    ASSERT(bus_info_256.has_value() && bus_info_256.value() == bus_info1);
    ASSERT(bus_info_750.has_value() && bus_info_750.value() == bus_info2);
}

void TransportCatalogueTests::GetStopInfo() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Domain::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Domain::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Domain::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Domain::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Domain::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Domain::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Domain::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(
            Domain::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(
            Domain::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5,
                    27400));
    transport_catalogue.InsertBus(Domain::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
    auto& buses = transport_catalogue.GetBuses();
    
    Domain::StopInfo stop_info_2{"Prazhskaya", {}};
    Domain::StopInfo stop_info_3{"Biryulyovo Zapadnoye", {&buses[0], &buses[2]}};
    
    std::optional<Domain::StopInfo> stop_info_samara = transport_catalogue.GetStopInfo(nullptr);
    std::optional<Domain::StopInfo> stop_info_prazhskaya = transport_catalogue.GetStopInfo(&stops[9]);
    std::optional<Domain::StopInfo> stop_info_birul = transport_catalogue.GetStopInfo(&stops[3]);
    
    ASSERT(!stop_info_samara.has_value());
    ASSERT(stop_info_prazhskaya.has_value() && stop_info_prazhskaya.value() == stop_info_2);
    ASSERT(stop_info_birul.has_value() && stop_info_birul.value() == stop_info_3);
}

void StreamReaderTests::Load() {
    std::istringstream file_input_stream("13\n"
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
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::StreamReader stream_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = stream_reader;
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    
    std::deque<Domain::Bus>& buses = transport_catalogue.GetBuses();
    ASSERT(buses.size() == 3);
    std::deque<Domain::Stop>& stops = transport_catalogue.GetStops();
    ASSERT(stops.size() == 10);
    Domain::Stop stop("Biryulyovo Tovarnaya", 55.592028, 37.653656);
    ASSERT(transport_catalogue.FindStop("Biryulyovo Tovarnaya").has_value() && *transport_catalogue.FindStop(
            "Biryulyovo Tovarnaya").value() == stop);
    
    Domain::Bus bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5,
            27400);
    ASSERT((transport_catalogue.FindBus("750").has_value() && *transport_catalogue.FindBus("750").value() == bus));
    ASSERT(transport_catalogue.GetBusRealLength(transport_catalogue.FindBus("750").value()->route) == 27400);
}

void StreamReaderTests::SendAnswer() {
    TransportCatalogue transport_catalogue{};
    transport_catalogue.InsertStop(Domain::Stop("Tolstopaltsevo", 55.611087, 37.208290));
    transport_catalogue.InsertStop(Domain::Stop("Marushkino", 55.595884, 37.209755));
    transport_catalogue.InsertStop(Domain::Stop("Rasskazovka", 55.632761, 37.333324));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Zapadnoye", 55.574371, 37.651700));
    transport_catalogue.InsertStop(Domain::Stop("Biryusinka", 55.581065, 37.648390));
    transport_catalogue.InsertStop(Domain::Stop("Universam", 55.587655, 37.645687));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Tovarnaya", 55.592028, 37.653656));
    transport_catalogue.InsertStop(Domain::Stop("Biryulyovo Passazhirskaya", 55.580999, 37.659164));
    transport_catalogue.InsertStop(Domain::Stop("Rossoshanskaya ulitsa", 55.595579, 37.605757));
    transport_catalogue.InsertStop(Domain::Stop("Prazhskaya", 55.611678, 37.603831));
    auto& stops = transport_catalogue.GetStops();
    transport_catalogue.InsertBus(
            Domain::Bus("256", {&stops[3], &stops[4], &stops[5], &stops[6], &stops[7], &stops[3]}, 4371.02, 5950));
    transport_catalogue.InsertBus(
            Domain::Bus("750", {&stops[0], &stops[1], &stops[1], &stops[2], &stops[1], &stops[1], &stops[0]}, 20939.5,
                    27400));
    transport_catalogue.InsertBus(Domain::Bus("828", {&stops[3], &stops[5], &stops[8], &stops[3]}, 14431.0, 15500.0));
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
    
    std::istringstream file_input_stream("6\n"
                                         "Bus 256\n"
                                         "Bus 750\n"
                                         "Bus 751\n"
                                         "Stop Samara\n"
                                         "Stop Prazhskaya\n"
                                         "Stop Biryulyovo Zapadnoye "s);
    std::ostringstream o_string_stream;
    
    IoRequests::StreamReader stream_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = stream_reader;
    
    input_reader.PreloadDocument();
    input_reader.SendAnswer();
    
    std::string correct_answer = "Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature\n"
                                 "Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature\n"
                                 "Bus 751: not found\n"
                                 "Stop Samara: not found\n"
                                 "Stop Prazhskaya: no buses\n"
                                 "Stop Biryulyovo Zapadnoye: buses 256 828\n"s;
    const auto& str = o_string_stream.str();
    ASSERT(str == correct_answer);
}

void MapRenderTests::TestCase1() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_01_input.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    auto answer = o_string_stream.str();
/*
        std::cout << answer << std::endl;
*/
    ASSERT(true);

}

void MapRenderTests::TestCase2() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_02_input.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    auto answer = o_string_stream.str();
/*
    std::cout << answer << std::endl << std::endl;
*/
    ASSERT(true);

}

void MapRenderTests::TestCase3() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_03_input.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    auto answer = o_string_stream.str();
/*
    std::cout << answer << std::endl << std::endl;
*/
    ASSERT(true);

}

void MapRenderTests::TestCaseUnicnownStopPureCatalog() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_04_input.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    auto answer = o_string_stream.str();
    std::string correct_answer = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n</svg>";
/*
    std::cout << answer << std::endl << std::endl;
*/
    ASSERT(answer == correct_answer);

}

void MapRenderTests::TestCaseBigData() {
    std::ifstream file_input_stream(getexepath() + "/test_case/maprender_case_big_data_input.json");
    std::ifstream file_output_stream(getexepath() + "/test_case/maprender_case_big_data_output.json");
    std::ostringstream o_string_stream;
    
    TransportCatalogue transport_catalogue{};
    IoRequests::JsonReader json_reader(transport_catalogue, file_input_stream, o_string_stream);
    IoRequests::IoBase& input_reader = json_reader;
    IoRequests::IRenderSettings& render_settings = json_reader;
    renderer::MapRenderer map_renderer(transport_catalogue, o_string_stream);
    
    input_reader.PreloadDocument();
    input_reader.LoadData();
    map_renderer.CreateDocument(render_settings.GetRenderSettings());
    map_renderer.Render();
    
    auto answer = o_string_stream.str() + "\n";
    std::string correct_answer = GetTextFromStream(file_output_stream);
    
/*
    std::cout << correct_answer << std::endl << std::endl;
    std::cout << answer << std::endl << std::endl;
*/
    
    ASSERT(answer == correct_answer);
}


void AllTests() {
    IntegrationTests integration_tests;
    RUN_TEST(integration_tests.TestCase_5_PlusRealRoutersAndCurveInBusInformation)
    RUN_TEST(integration_tests.TestCase_6_JsonReader)
    RUN_TEST(integration_tests.TestCase_7_MapRender)
    TransportCatalogueTests transport_catalogue_tests;
    RUN_TEST(transport_catalogue_tests.TrackSectionHasher)
    RUN_TEST(transport_catalogue_tests.AddBus)
    RUN_TEST(transport_catalogue_tests.AddStop)
    RUN_TEST(transport_catalogue_tests.FindBus)
    RUN_TEST(transport_catalogue_tests.FindStop)
    RUN_TEST(transport_catalogue_tests.GetBusInfo)
    RUN_TEST(transport_catalogue_tests.GetBusInfoPlusCurvatureAdded)
    RUN_TEST(transport_catalogue_tests.GetStopInfo)
    StreamReaderTests stream_reader_tests;
    RUN_TEST(stream_reader_tests.Load)
    RUN_TEST(stream_reader_tests.SendAnswer)
    MapRenderTests map_render_tests;
    RUN_TEST(map_render_tests.TestCase1);
    RUN_TEST(map_render_tests.TestCase2);
    RUN_TEST(map_render_tests.TestCase3);
    RUN_TEST(map_render_tests.TestCaseUnicnownStopPureCatalog);
    RUN_TEST(map_render_tests.TestCaseBigData);
}

}