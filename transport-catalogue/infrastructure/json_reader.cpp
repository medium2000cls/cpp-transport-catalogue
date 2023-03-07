#include <memory>
#include <algorithm>
#include "json_reader.h"

namespace TransportGuide::IoRequests {
using namespace std::literals;

JsonReader::JsonReader(TransportGuide::BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream,
        std::ostream& output_stream) : IoRequestsBase(catalogue), input_stream_(input_stream), output_stream_(
        output_stream) {
}

void JsonReader::LoadData() {
    document_ = json::Load(input_stream_);
    
    const json::Node& root_node = document_.GetRoot();
    if (!(root_node.IsMap() && root_node.AsMap().count("base_requests"))) { return; }
    
    const json::Node& base_requests_node = root_node.AsMap().at("base_requests");
    if (!(base_requests_node.IsArray() && !base_requests_node.AsArray().empty())) { return; }
    
    std::vector<const json::Node*> stop_requests;
    std::vector<const json::Node*> bus_requests;
    
    for (const json::Node& node : base_requests_node.AsArray()) {
        if (node.IsMap() && node.AsMap().count("type")) {
            const json::Node& type_node = node.AsMap().at("type");
            if (type_node == "Stop"s) {
                stop_requests.push_back(&node);
            }
            else if (type_node == "Bus"s) {
                bus_requests.push_back(&node);
            }
            else if (type_node.IsNull()) {
                continue;
            }
            else {
                throw std::logic_error("Node key \"type\" must be count value \"Stop\" or \"Bus\"."s);
            }
        }
        else {
            throw std::logic_error("Node is not count key \"type\"."s);
        }
    }
    std::for_each(stop_requests.begin(), stop_requests.end(), [this](const json::Node* node_ptr) {
        AddStopByNode(node_ptr);
    });
    std::for_each(bus_requests.begin(), bus_requests.end(), [this](const json::Node* node_ptr) {
        AddBusByNode(node_ptr);
    });
}

void JsonReader::AddStopByNode(const json::Node* node_ptr) {
    node_ptr->IsMap() ? 0 : throw std::logic_error("Json Stop node must be Dictionary(key,Node)."s);
    
    const json::Dict& node_dict = node_ptr->AsMap();
    
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json Stop node must be contains \"type\"."s);
    node_dict.at("type"s) == "Stop"s ? 0 : throw std::logic_error("Key \"type\" must be contains value \"Stop\"."s);
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json Stop node must be contains \"name\"."s);
    node_dict.count("latitude"s) ? 0 : throw std::logic_error("Json Stop node must be contains \"latitude\"."s);
    node_dict.count("longitude"s) ? 0 : throw std::logic_error("Json Stop node must be contains \"longitude\"."s);
    node_dict.count("road_distances"s) ? 0 : throw std::logic_error(
            "Json Stop node must be contains \"road_distances\"."s);
    node_dict.at("road_distances"s).IsMap() ? 0 : throw std::logic_error(
            "Key \"road_distances\" must be Dictionary(key,Node)."s);
    
    std::string name = node_dict.at("name").AsString();
    double latitude = node_dict.at("latitude").AsDouble();
    double longitude = node_dict.at("longitude").AsDouble();
    Domain::Stop* stop_ptr = catalogue_.InsertStop(Domain::Stop(name, latitude, longitude));
    
    for (const auto& [key, value] : node_dict.at("road_distances"s).AsMap()) {
        double distance = value.AsDouble();
        std::string to_stop_name = key;
        Domain::Stop* to_stop_ptr = catalogue_.InsertStop(Domain::Stop(to_stop_name));
        catalogue_.AddRealDistanceToCatalog(stop_ptr, to_stop_ptr, distance);
    }
}

void JsonReader::AddBusByNode(const json::Node* node_ptr) {
    node_ptr->IsMap() ? 0 : throw std::logic_error("Json Bus node must be Dictionary(key,Node)."s);
    
    const json::Dict& node_dict = node_ptr->AsMap();
    
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json Bus node must be contains \"type\"."s);
    node_dict.at("type"s) == "Bus"s ? 0 : throw std::logic_error("Key \"type\" must be contains value \"Bus\"."s);
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json Bus node must be contains \"name\"."s);
    node_dict.count("stops"s) ? 0 : throw std::logic_error("Json Bus node must be contains \"stops\"."s);
    node_dict.at("stops"s).IsArray() ? 0 : throw std::logic_error("Key \"stops\" must be array."s);
    node_dict.count("is_roundtrip"s) ? 0 : throw std::logic_error("Json Bus node must be contains \"is_roundtrip\"."s);
    node_dict.at("is_roundtrip"s).IsBool() ? 0 : throw std::logic_error("Key \"is_roundtrip\" must be bool."s);
    
    std::string bus_name = node_dict.at("name").AsString();
    std::vector<const Domain::Stop*> route;
    
    for (const auto& stop_node : node_dict.at("stops"s).AsArray()) {
        std::string stop_name = stop_node.AsString();
        Domain::Stop* stop_ptr = catalogue_.InsertStop(Domain::Stop(stop_name));
        route.push_back(stop_ptr);
    }
    
    if (!node_dict.at("is_roundtrip"s).AsBool()) {
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

void JsonReader::SendAnswer() {
    const auto& root_node = document_.GetRoot();
    if (!(root_node.IsMap() && root_node.AsMap().count("stat_requests"))) { return; }
    
    const json::Node& stat_requests_node = root_node.AsMap().at("stat_requests");
    if (!(stat_requests_node.IsArray() && !stat_requests_node.AsArray().empty())) { return; }
    
    json::Array answer_array;
    
    for (const auto& node : stat_requests_node.AsArray()) {
        node.IsMap() ? 0 : throw std::logic_error("Json request node must be Dictionary(key,Node)."s); //todo Modify exception text
        
        const json::Node& type_node = node.AsMap().at("type");
        if (type_node == "Stop"s) {
            answer_array.push_back(GetStopRequestNode(node));
        }
        else if (type_node == "Bus"s) {
            answer_array.push_back(GetBusRequestNode(node));
        }
        else if (type_node.IsNull()) {
            continue;
        }
        else {
            throw std::logic_error("Node key \"type\" must be count value \"Stop\" or \"Bus\"."s);
        }
    }
    
    json::Print(json::Document(answer_array), output_stream_);
}

json::Node JsonReader::GetStopRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);  //todo Modify exception text
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);  //todo Modify exception text
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json request node must be contains \"name\"."s);  //todo Modify exception text
    
    const std::string& name = node_dict.at("name"s).AsString();
    
    auto stop_info = catalogue_.GetStopInfo(name);
    
    json::Dict result_node;
    result_node["request_id"] = node_dict.at("id");
    
    if (stop_info.has_value() /*&& !stop_info.value().buses.empty()*/) {
        json::Array buses;
        for (const auto& bus : stop_info.value().buses){
            buses.emplace_back(bus->name);            //todo возможна ошибка из-за сортировки
        }
        result_node["buses"] = buses;
    }
/*
    else if (stop_info.has_value()) {
        result_node["error_message"] = "no buses"s;
    }
*/
    else {
        result_node["error_message"] = "not found"s;
    }
    return result_node;
}

json::Node JsonReader::GetBusRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);  //todo Modify exception text
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);  //todo Modify exception text
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json request node must be contains \"name\"."s);  //todo Modify exception text
    
    std::string bus_name = node_dict.at("name"s).AsString();
    auto bus_info = catalogue_.GetBusInfo(bus_name);
    
    json::Dict result_node;
    result_node["request_id"] = node_dict.at("id");
    
    if (bus_info.has_value()) {
        result_node["curvature"] = bus_info.value().curvature;
        result_node["route_length"] = bus_info.value().length;
        result_node["stop_count"] = static_cast<int>(bus_info.value().stops_count);
        result_node["unique_stop_count"] = static_cast<int>(bus_info.value().unique_stops_count);
    }
    else {
        result_node["error_message"] = "not found"s;
    }
    return result_node;
}
}