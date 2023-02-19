//
// Created by Дмитрий on 07.02.2023.
//
#include <string>
#include <iostream>
#include <map>
#include <unordered_set>
#include <algorithm>
#include "input_reader.h"

namespace TransportGuide::IoRequests {
using namespace std::literals;

BaseInputReader::BaseInputReader(Core::TransportCatalogue& catalogue) : catalogue_(catalogue) {}

StreamInputReader::StreamInputReader(Core::TransportCatalogue& catalogue, std::istream& input_stream) : BaseInputReader(
        catalogue), input_stream_(input_stream) {}

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
    if (str.find_first_of(separator) == str.npos) {
        str.remove_prefix(str.size());
    }
    else {
        str.remove_prefix(str.find_first_not_of(separator, str.find_first_of(separator)));
        str.remove_prefix(str.find_first_not_of(" \n"));
    }
    return static_cast<std::string>(sub_str);
};

void StreamInputReader::Load() {
    std::vector<std::string> stop_requests;
    std::vector<std::string> bus_requests;
    
    std::string str;
    std::getline(input_stream_, str);
    size_t count_str = std::stoul(str);
    
    for (size_t i = 0; i < count_str; ++i) {
        std::getline(input_stream_, str);
        std::string_view str_view = str;
        
        std::string code_str = extract_sub_str(str_view, " ");
        if (code_str == "Stop") {
            stop_requests.emplace_back(str_view);
        }
        else if (code_str == "Bus") {
            bus_requests.emplace_back(str_view);
        }
    }
    
    for_each(stop_requests.begin(),stop_requests.end(), [this](const std::string& str){
        AddStopByDescription(str);
    });
    for_each(bus_requests.begin(),bus_requests.end(), [this](const std::string& str){
        AddBusByDescription(str);
    });
}

void StreamInputReader::AddStopByDescription(std::string_view str_view) {
    std::string name = extract_sub_str(str_view, ":");
    double latitude = std::stod(extract_sub_str(str_view, ","));
    double longitude = std::stod(extract_sub_str(str_view, ", \n"));
    Core::Stop* stop_ptr= catalogue_.InsertStop(Core::Stop(name, latitude, longitude));
    
    while (!str_view.empty()){
        double distance = std::stod(extract_sub_str(str_view, "m"));
        extract_sub_str(str_view, " ");
        std::string to_stop_name = extract_sub_str(str_view, ",");
        Core::Stop* to_stop_ptr = catalogue_.InsertStop(Core::Stop(to_stop_name));
        catalogue_.AddRealDistanceToCatalog(stop_ptr, to_stop_ptr, distance);
    }
}

void StreamInputReader::AddBusByDescription(std::string_view str_view) {
    std::string bus_name = extract_sub_str(str_view, ":");
    std::vector<const Core::Stop*> route;
    auto fill_route = [&](const std::string& separator) {
        while (!str_view.empty()) {
            std::string stop_name = extract_sub_str(str_view, separator);
            Core::Stop* stop_ptr = catalogue_.InsertStop(Core::Stop(stop_name));
            route.push_back(stop_ptr);
        }
    };
    if (str_view.find('>') != std::string::npos) {
        fill_route(">\n");
    }
    else {
        fill_route("-\n");
        std::vector<const Core::Stop*> backward_route;
        std::copy(std::next(route.rbegin()), route.rend(),
                std::back_insert_iterator<std::vector<const Core::Stop*>>(backward_route));
        std::move(backward_route.begin(), backward_route.end(),
                std::back_insert_iterator<std::vector<const Core::Stop*>>(route));
    }
    double calc_dist = catalogue_.GetBusCalculateLength(route);
    double real_dist = catalogue_.GetBusRealLength(route);
    catalogue_.InsertBus(Core::Bus(bus_name, route, calc_dist, real_dist));
}

}
