#include <string>
#include <iostream>
#include <map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include "stream_reader.h"

namespace TransportGuide::IoRequests {

using namespace std::literals;

auto trim = [](std::string_view&& str, const std::string& separator) {
    str.remove_prefix(str.find_first_not_of(separator));
    str.remove_suffix(str.size() - str.find_last_not_of(separator) - 1);
    return static_cast<std::string>(str);
};

inline auto extract_sub_str = [](std::string_view& str, const std::string& separator) {
    auto sub_str = str.substr(0, str.find_first_of(separator));
    sub_str.remove_prefix(sub_str.find_first_not_of(" \n"));
    sub_str.remove_suffix(sub_str.size() - sub_str.find_last_not_of(" \n") - 1);
    //std::cout << sub_str << ", ";
    if (str.find_first_of(separator) == std::string_view::npos) {
        str.remove_prefix(str.size());
    }
    else {
        str.remove_prefix(str.find_first_not_of(separator, str.find_first_of(separator)));
        str.remove_prefix(str.find_first_not_of(" \n"));
    }
    return static_cast<std::string>(sub_str);
};


StreamReader::StreamReader(BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream,
        std::ostream& output_stream) : IoBase(catalogue), input_stream_(input_stream), output_stream_(
        output_stream), input_string_stream_() {}

void StreamReader::PreloadDocument() {
    document_ = std::string (std::istreambuf_iterator<char>(input_stream_), {});
    input_string_stream_ = std::istringstream(document_);
}

void StreamReader::LoadData() {
    std::vector<std::string> stop_requests;
    std::vector<std::string> bus_requests;
    
    std::string str;
    std::getline(input_string_stream_, str);
    size_t count_str = std::stoul(str);
    
    for (size_t i = 0; i < count_str; ++i) {
        std::getline(input_string_stream_, str);
        std::string_view str_view = str;
        
        std::string code_str = extract_sub_str(str_view, " ");
        if (code_str == "Stop") {
            stop_requests.emplace_back(str_view);
        }
        else if (code_str == "Bus") {
            bus_requests.emplace_back(str_view);
        }
    }
    
    for_each(stop_requests.begin(), stop_requests.end(), [this](const std::string& str) {
        AddStopByDescription(str);
    });
    for_each(bus_requests.begin(), bus_requests.end(), [this](const std::string& str) {
        AddBusByDescription(str);
    });
}

void StreamReader::AddStopByDescription(std::string_view str_view) {
    std::string name = extract_sub_str(str_view, ":");
    double latitude = std::stod(extract_sub_str(str_view, ","));
    double longitude = std::stod(extract_sub_str(str_view, ", \n"));
    Domain::Stop* stop_ptr = catalogue_.InsertStop(Domain::Stop(name, latitude, longitude));
    
    while (!str_view.empty()) {
        double distance = std::stod(extract_sub_str(str_view, "m"));
        extract_sub_str(str_view, " ");
        std::string to_stop_name = extract_sub_str(str_view, ",");
        Domain::Stop* to_stop_ptr = catalogue_.InsertStop(Domain::Stop(to_stop_name));
        catalogue_.AddRealDistanceToCatalog(stop_ptr, to_stop_ptr, distance);
    }
}

void StreamReader::AddBusByDescription(std::string_view str_view) {
    std::string bus_name = extract_sub_str(str_view, ":");
    std::vector<const Domain::Stop*> route;
    auto fill_route = [&](const std::string& separator) {
        while (!str_view.empty()) {
            std::string stop_name = extract_sub_str(str_view, separator);
            Domain::Stop* stop_ptr = catalogue_.InsertStop(Domain::Stop(stop_name));
            route.push_back(stop_ptr);
        }
    };
    if (str_view.find('>') != std::string::npos) {
        fill_route(">\n");
    }
    else {
        fill_route("-\n");
        std::vector<const Domain::Stop*> backward_route;
        std::copy(std::next(route.rbegin()), route.rend(),
                std::back_insert_iterator<std::vector<const Domain::Stop*>>(backward_route));
        std::move(backward_route.begin(), backward_route.end(),
                std::back_insert_iterator<std::vector<const Domain::Stop*>>(route));
    }
    double calc_dist = catalogue_.GetBusCalculateLength(route);
    double real_dist = catalogue_.GetBusRealLength(route);
    catalogue_.InsertBus(Domain::Bus(bus_name, route, calc_dist, real_dist));
}

std::ostream& operator<< (std::ostream& o_stream, const Domain::BusInfo& bus_info) {
    o_stream << "Bus " << bus_info.name << ": " << bus_info.stops_count << " stops on route, "
            << bus_info.unique_stops_count << " unique stops, " << bus_info.length << " route length, "
            << bus_info.curvature << " curvature";
    return o_stream;
}

std::ostream& operator<< (std::ostream& o_stream, const std::vector<const Domain::Bus*>& buses) {
    for (auto bus : buses){
        o_stream << " " << bus->name;
    }
    return o_stream;
}

std::ostream& operator<< (std::ostream& o_stream, const Domain::StopInfo& stop_info) {
    o_stream << "Stop " << stop_info.name << ": buses" << stop_info.buses;
    return o_stream;
}

void StreamReader::SendAnswer() {
    std::string str;
    std::getline(input_string_stream_, str);
    size_t count_str = std::stoul(str);
    
    for (size_t i = 0; i < count_str; ++i) {
        std::getline(input_string_stream_, str);
        std::string_view str_view = str;
        
        std::string code_str = extract_sub_str(str_view, " ");
        if (code_str == "Stop") {
            std::string stop_name = extract_sub_str(str_view, "\n");
            auto stop_info = catalogue_.GetStopInfo(stop_name);
            if (stop_info.has_value() && !stop_info.value().buses.empty()) {
                output_stream_ << stop_info.value() << std::endl;
            }
            else if (stop_info.has_value()) {
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

}
