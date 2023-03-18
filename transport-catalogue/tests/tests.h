#pragma once

#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <chrono>

#include "../business_logic/transport_catalogue.h"
#include "../domain/domain.h"


namespace TransportGuide::Test {

//region assist
class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;
    
    LogDuration(std::string_view id, std::ostream& dst_stream = std::cerr) : id_(id), dst_stream_(dst_stream) {
    }
    
    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;
        
        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        dst_stream_ << id_ << ": "sv << duration_cast<milliseconds>(dur).count() << " ms"sv << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& dst_stream_;
};

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container) {
    using namespace std;
    bool is_first = true;
    out << "["s;
    for (const T& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
    out << "]"s;
    return out;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::set<T>& container) {
    using namespace std;
    bool is_first = true;
    out << "{"s;
    for (const T& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
    out << "}"s;
    return out;
}

template<typename K, typename T>
std::ostream& operator<<(std::ostream& out, const std::map<K, T>& container) {
    using namespace std;
    bool is_first = true;
    out << "{"s;
    for (const auto& [key, element] : container) {
        if (is_first) {
            out << key << ": " << element;
            is_first = false;
        }
        else {
            out << ", "s << key << ": " << element;
        }
    }
    out << "}"s;
    return out;
}

template<typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
        const std::string& file, const std::string& func, unsigned int line, const std::string& hint) {
    using namespace std;
    
    if (static_cast<int>(t) != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func,
        unsigned line, const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void RunTest(const std::string& func_name);
void AbortTest(const std::string& func_name, std::string exception_text = "");

#define RUN_TEST(func) {             \
    try {                            \
        func();                      \
        RunTest(#func);              \
    }                                \
    catch (const std::exception& e) {\
    AbortTest(#func, e.what());      \
    }                                \
    catch (...){ AbortTest(#func); } \
}

//endregion

class TransportCatalogue final : public TransportGuide::BusinessLogic::TransportCatalogue {
public:
    void AddRealDistanceToCatalog(Domain::TrackSection track_section, double distance);
    double GetBusRealLength(const std::vector<const Domain::Stop*>& route);
    std::deque<Domain::Stop>& GetStops();
    std::deque<Domain::Bus>& GetBuses();
};


class IntegrationTests {
public:
    void TestCase_5_PlusRealRoutersAndCurveInBusInformation();
    void TestCase_6_JsonReader();
    void TestCase_7_MapRender();
};


class TransportCatalogueTests {
public:
    void TrackSectionHasher();
    void AddBus();
    void AddStop();
    void FindBus();
    void FindStop();
    void GetBusInfo();
    void GetBusInfoPlusCurvatureAdded();
    void GetStopInfo();
};


class StreamReaderTests {
public:
    void Load();
    void SendAnswer();
};

class MapRenderTests {
public:
    void TestCase1();
    void TestCase2();
    void TestCase3();
    void TestCaseUnicnownStopPureCatalog();
    void TestCaseBigData();
   
};

void AllTests();

}