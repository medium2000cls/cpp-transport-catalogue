#include "stat_reader.h"

namespace TransportGuide::IoRequests {

BaseStatReader::BaseStatReader(Core::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

inline auto extract_sub_str = [](std::string_view& str, const std::string& separator) {
    auto sub_str = str.substr(0, str.find_first_of(separator));
    sub_str.remove_prefix(sub_str.find_first_not_of(" \n"));
    sub_str.remove_suffix(sub_str.size() - sub_str.find_last_not_of(" \n") - 1);
    if (str.find_first_of(separator) == str.npos) {
        str.remove_prefix(str.size());
    }
    else {
        str.remove_prefix(str.find_first_not_of(separator, str.find_first_of(separator)));
        str.remove_prefix(str.find_first_not_of(" \n"));
    }
    return static_cast<std::string>(sub_str);
};

std::ostream& operator<< (std::ostream& o_stream, const Core::BusInfo& bus_info) {
    o_stream << "Bus " << bus_info.name << ": " << bus_info.stops_count << " stops on route, "
            << bus_info.unique_stops_count << " unique stops, " << bus_info.length << " route length, "
            << bus_info.curvature << " curvature";
    return o_stream;
}

std::ostream& operator<< (std::ostream& o_stream, const std::vector<const Core::Bus*>& buses) {
    for (auto bus : buses){
        o_stream << " " << bus->name;
    }
    return o_stream;
}

std::ostream& operator<< (std::ostream& o_stream, const Core::StopInfo& stop_info) {
    o_stream << "Stop " << stop_info.name << ": buses" << stop_info.buses;
    return o_stream;
}

void StreamStatReader::SendAnswer() {
    std::string str;
    std::getline(input_stream_, str);
    size_t count_str = std::stoul(str);
    
    for (size_t i = 0; i < count_str; ++i) {
        std::getline(input_stream_, str);
        std::string_view str_view = str;
        
        std::string code_str = extract_sub_str(str_view, " ");
        if (code_str == "Stop") {
            std::string stop_name = extract_sub_str(str_view, "\n");
            auto stop_info = catalogue_.GetStopInfo(stop_name);
            if (stop_info.has_value() && !stop_info.value().buses.empty()) {
                output_stream_ << stop_info.value() << std::endl;
            }
            else if (stop_info.has_value()){
                output_stream_ << "Stop " << stop_name << ": no buses" << std::endl;
            }
            else {
                output_stream_ << "Stop " << stop_name << ": not found" << std::endl;
            }
        }
        else if (code_str == "Bus") {
            std::string bus_name = extract_sub_str(str_view, "\n");
            auto bus_info = catalogue_.GetBusInfo(bus_name);
            if (bus_info.has_value()) {
                output_stream_ << bus_info.value() << std::endl;
            }
            else {
                output_stream_ << "Bus " << bus_name << ": not found" << std::endl;
            }
        }
    }
}


StreamStatReader::StreamStatReader(Core::TransportCatalogue& catalogue, std::istream& input_stream,
        std::ostream& output_stream) : BaseStatReader(catalogue), input_stream_(input_stream), output_stream_(
        output_stream) {}
    
}
