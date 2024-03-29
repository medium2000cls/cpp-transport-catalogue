#include <memory>
#include <algorithm>
#include <cassert>
#include "json_reader.h"


namespace TransportGuide::IoRequests {
using namespace std::literals;

JsonReader::JsonReader(renderer::MapRenderer& map_renderer,
                       TransportGuide::BusinessLogic::TransportCatalogue& catalogue, std::istream& input_stream,
                       std::ostream& output_stream)
    : IoBase(catalogue), RenderBase(map_renderer), input_stream_(input_stream), output_stream_(output_stream) {
}

void JsonReader::PreloadDocument() {
    document_ = json::Load(input_stream_);
}

void JsonReader::LoadData() {
    assert(document_.GetRoot() != json::Node());
    const json::Node& root_node = document_.GetRoot();
    if (!(root_node.IsMap() && root_node.AsMap().count("base_requests"s))) { return; }
    
    const json::Node& base_requests_node = root_node.AsMap().at("base_requests"s);
    if (!(base_requests_node.IsArray() && !base_requests_node.AsArray().empty())) { return; }
    
    std::vector<const json::Node*> stop_requests;
    std::vector<const json::Node*> bus_requests;
    
    for (const json::Node& node : base_requests_node.AsArray()) {
        if (node.IsMap() && node.AsMap().count("type")) {
            const json::Node& type_node = node.AsMap().at("type");
            if (type_node == "Stop"s) {
                stop_requests.push_back(&node);
            } else if (type_node == "Bus"s) {
                bus_requests.push_back(&node);
            } else if (type_node.IsNull()) {
                continue;
            } else {
                throw std::logic_error("Node key \"type\" must be count value \"Stop\" or \"Bus\"."s);
            }
        } else {
            throw std::logic_error("Node is not count key \"type\"."s);
        }
    }
    std::for_each(stop_requests.begin(), stop_requests.end(), [this](const json::Node* node_ptr) {
        AddStopByNode(*node_ptr);
    });
    std::for_each(bus_requests.begin(), bus_requests.end(), [this](const json::Node* node_ptr) {
        AddBusByNode(*node_ptr);
    });
    
    if (root_node.IsMap() && root_node.AsMap().count("routing_settings"s)) {
        Domain::RoutingSettings routing_settings = GetRoutingSettings(root_node.AsMap().at("routing_settings"s));
        catalogue_.ConstructUserRouteManager(routing_settings);
    }
    
    if (root_node.IsMap() && root_node.AsMap().count("render_settings"s)) {
        Domain::RenderSettings render_settings = GetRenderSettings(root_node.AsMap().at("render_settings"s));
        map_renderer_.SetRenderSettings(render_settings);
    }
}

void JsonReader::AddStopByNode(const json::Node& node_ptr) {
    node_ptr.IsMap() ? 0 : throw std::logic_error("Json Stop node must be Dictionary(key,Node)."s);
    
    const json::Dict& node_dict = node_ptr.AsMap();
    
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

void JsonReader::AddBusByNode(const json::Node& node_ptr) {
    node_ptr.IsMap() ? 0 : throw std::logic_error("Json Bus node must be Dictionary(key,Node)."s);
    
    const json::Dict& node_dict = node_ptr.AsMap();
    
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
        const std::string& stop_name = stop_node.AsString();
        Domain::Stop* stop_ptr = catalogue_.InsertStop(Domain::Stop(stop_name));
        route.push_back(stop_ptr);
    }
    size_t number_final_stop = 0;
    if (!node_dict.at("is_roundtrip"s).AsBool()) {
        number_final_stop = route.size();
        std::vector<const Domain::Stop*> backward_route;
        std::copy(std::next(route.rbegin()), route.rend(),
                std::back_insert_iterator<std::vector<const Domain::Stop*>>(backward_route));
        std::move(backward_route.begin(), backward_route.end(),
                std::back_insert_iterator<std::vector<const Domain::Stop*>>(route));
    }
    
    double calc_dist = catalogue_.GetBusCalculateLength(route);
    double real_dist = catalogue_.GetBusRealLength(route);
    catalogue_.InsertBus(Domain::Bus(bus_name, route, number_final_stop, calc_dist, real_dist));
}

Domain::RoutingSettings JsonReader::GetRoutingSettings(const json::Node& node_ptr) {
    node_ptr.IsMap() ? 0 : throw std::logic_error("Json Bus node must be Dictionary(key,Node)."s);
    
    const json::Dict& node_dict = node_ptr.AsMap();
    
    node_dict.count("bus_wait_time"s) ? 0 : throw std::logic_error("Json routing_settings node must be contains \"bus_wait_time\"."s);
    node_dict.at("bus_wait_time"s).IsInt() ? 0 : throw std::logic_error("Key \"bus_wait_time\" must be int."s);
    node_dict.count("bus_velocity"s) ? 0 : throw std::logic_error("Json routing_settings node must be contains \"bus_velocity\"."s);
    node_dict.at("bus_velocity"s).IsDouble() ? 0 : throw std::logic_error("Key \"bus_velocity\" must be double."s);
    
    return {.bus_wait_time = static_cast<Domain::TimeMinuts>(node_dict.at("bus_wait_time"s).AsInt()),.bus_velocity = node_dict.at("bus_velocity"s).AsDouble()};
}

void JsonReader::SendAnswer() {
    assert(document_.GetRoot() != json::Node());
    const auto& root_node = document_.GetRoot();
    if (!(root_node.IsMap() && root_node.AsMap().count("stat_requests"))) { return; }
    
    const json::Node& stat_requests_node = root_node.AsMap().at("stat_requests");
    if (!(stat_requests_node.IsArray() && !stat_requests_node.AsArray().empty())) { return; }
    
    json::Array answer_array;
    
    for (const auto& node : stat_requests_node.AsArray()) {
        node.IsMap() ? 0 : throw std::logic_error("Json request node must be Dictionary(key,Node)."s);
        
        const json::Node& type_node = node.AsMap().at("type");
        if (type_node == "Stop"s) {
            answer_array.push_back(GetStopRequestNode(node));
        } else if (type_node == "Bus"s) {
            answer_array.push_back(GetBusRequestNode(node));
        } else if (type_node == "Map"s) {
            answer_array.push_back(GetMapRequestNode(node));
        } else if (type_node == "Route"s) {
            answer_array.push_back(GetRouteRequestNode(node));
        }
//        else if (type_node.IsNull()) {
//            continue;
//        }
        else {
            throw std::logic_error("Node key \"type\" must be count value \"Stop\" or \"Bus\" or \"Map\"."s);
        }
    }
    
    json::Print(json::Document(std::move(answer_array)), output_stream_);
}

json::Node JsonReader::GetStopRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json request node must be contains \"name\"."s);
    
    const std::string& name = node_dict.at("name"s).AsString();
    
    auto stop_info = catalogue_.GetStopInfo(name);
    
    auto builder = json::Builder{};
    auto sub_dict_result = builder.StartDict();
    sub_dict_result.Key("request_id").Value(node_dict.at("id"));
    
    if (stop_info.has_value() /*&& !stop_info.value().buses.empty()*/) {
        auto sub_array_result = sub_dict_result.Key("buses").StartArray();
        for (const auto& bus : stop_info.value().buses) {
            sub_array_result.Value(bus->name);
        }
        sub_array_result.EndArray();
    }
    else {
        sub_dict_result.Key("error_message").Value("not found"s);
    }
    json::Node result = sub_dict_result.EndDict().Build();
    return result;
}

json::Node JsonReader::GetBusRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);
    node_dict.count("name"s) ? 0 : throw std::logic_error("Json request node must be contains \"name\"."s);
    
    std::string bus_name = node_dict.at("name"s).AsString();
    auto bus_info = catalogue_.GetBusInfo(bus_name);
    
    json::Builder builder = json::Builder{};
    auto sub_result = builder.StartDict().Key("request_id").Value(node_dict.at("id"));
    if (bus_info.has_value()) {
        sub_result.Key("curvature").Value(bus_info.value().curvature)
                  .Key("route_length").Value(bus_info.value().length)
                  .Key("stop_count").Value(static_cast<int>(bus_info.value().stops_count))
                  .Key("unique_stop_count").Value(static_cast<int>(bus_info.value().unique_stops_count));
    }
    else {
        sub_result.Key("error_message").Value("not found"s);
    }
    json::Node result = sub_result.EndDict().Build();
    return result;
}

json::Node JsonReader::GetMapRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);
    
    json::Node result_node = json::Builder{}.StartDict()
                                                .Key("request_id").Value(node_dict.at("id"))
                                                .Key("map").Value(Render())
                                            .EndDict()
                                            .Build();
    return result_node;
}

json::Node JsonReader::GetRouteRequestNode(const json::Node& node) {
    const json::Dict& node_dict = node.AsMap();
    node_dict.count("id"s) ? 0 : throw std::logic_error("Json request node must be contains \"id\"."s);
    node_dict.count("type"s) ? 0 : throw std::logic_error("Json request node must be contains \"type\"."s);
    node_dict.count("from"s) ? 0 : throw std::logic_error("Json request node must be contains \"from\"."s);
    node_dict.count("to"s) ? 0 : throw std::logic_error("Json request node must be contains \"to\"."s);
    
    std::string stop_from = node_dict.at("from"s).AsString();
    std::string stop_to = node_dict.at("to"s).AsString();;
    std::optional<Domain::UserRouteInfo> route_info = catalogue_.GetUserRouteManager().GetUserRouteInfo(stop_from, stop_to);
    
    json::Builder builder = json::Builder{};
    auto sub_result = builder.StartDict().Key("request_id").Value(node_dict.at("id"));
    
    if (route_info.has_value()) {
        
        sub_result.Key("total_time").Value(route_info.value().total_time);
        
        auto sub_array_result = sub_result.Key("items").StartArray();
        for (const auto& item : route_info.value().items) {
            if (std::holds_alternative<Domain::UserRouteInfo::UserWait>(item)) {
                const auto& user_wait = std::get<Domain::UserRouteInfo::UserWait>(item);
                sub_array_result.StartDict()
                                    .Key("type").Value("Wait")
                                    .Key("stop_name").Value(user_wait.stop->name)
                                    .Key("time").Value(user_wait.time)
                                .EndDict();
                
            } else if (std::holds_alternative<Domain::UserRouteInfo::UserBus>(item)) {
                const auto& user_bus = std::get<Domain::UserRouteInfo::UserBus>(item);
                sub_array_result.StartDict()
                                    .Key("type").Value("Bus")
                                    .Key("bus").Value(user_bus.bus->name)
                                    .Key("span_count").Value(static_cast<int>(user_bus.span_count))
                                    .Key("time").Value(user_bus.time)
                                .EndDict();
            }
        }
        sub_array_result.EndArray();
    }
    else {
        sub_result.Key("error_message").Value("not found"s);
    }
    
    json::Node result = sub_result.EndDict().Build();
    return result;
}

Domain::RenderSettings JsonReader::GetRenderSettings(const json::Node& render_settings_node) {
    if (!(render_settings_node.IsMap() && !render_settings_node.AsMap().empty())) {
        throw std::logic_error("\"render_settings\" is empty.");
    }
    const auto& map_render = render_settings_node.AsMap();
    
    svg::Color underlayer_color_variant;
    if (map_render.at("underlayer_color").IsString()) {
        underlayer_color_variant = map_render.at("underlayer_color").AsString();
    } else if (map_render.at("underlayer_color").IsArray()) {
        auto array_color = map_render.at("underlayer_color").AsArray();
        if (array_color.size() == 3) {
            underlayer_color_variant = svg::Rgb(array_color.at(0).AsInt(), array_color.at(1).AsInt(),
                                                array_color.at(2).AsInt());
        } else if (array_color.size() == 4) {
            underlayer_color_variant = svg::Rgba(array_color.at(0).AsInt(), array_color.at(1).AsInt(),
                                                 array_color.at(2).AsInt(), array_color.at(3).AsDouble());
        }
    } else {
        throw std::logic_error("There is no underlayer color.");
    }
    
    std::string underlayer_color = std::visit(svg::RenderColorAttribute{}, underlayer_color_variant);
    
    std::vector<std::string> color_palette_variant;
    if (map_render.at("color_palette").IsArray() && !map_render.at("color_palette").AsArray().empty()) {
        for (const auto& node : map_render.at("color_palette").AsArray()) {
            if (node.IsString()) {
                color_palette_variant.push_back(node.AsString());
            } else if (node.IsArray()) {
                auto array_color = node.AsArray();
                if (array_color.size() == 3) {
                    color_palette_variant.push_back(
                        std::visit(
                            svg::RenderColorAttribute{},
                            svg::Color(svg::Rgb(array_color.at(0).AsInt(),
                                                array_color.at(1).AsInt(),
                                                array_color.at(2).AsInt()))));
                } else if (array_color.size() == 4) {
                    color_palette_variant.push_back(
                        std::visit(
                            svg::RenderColorAttribute{},
                            svg::Color(svg::Rgba(array_color.at(0).AsInt(),
                                                 array_color.at(1).AsInt(),
                                                 array_color.at(2).AsInt(),
                                                 array_color.at(3).AsDouble()))));
                }
            }
        }
    } else {
        throw std::logic_error("There is no color palette.");
    }
    
    Domain::RenderSettings render_settings{map_render.at("width").AsDouble(), map_render.at("height").AsDouble(),
                                             map_render.at("padding").AsDouble(),
                                             map_render.at("line_width").AsDouble(),
                                             map_render.at("stop_radius").AsDouble(),
                                             map_render.at("bus_label_font_size").AsInt(),
                                             {map_render.at("bus_label_offset").AsArray().at(0).AsDouble(),
                                              map_render.at("bus_label_offset").AsArray().at(1).AsDouble()},
                                             map_render.at("stop_label_font_size").AsInt(),
                                             {map_render.at("stop_label_offset").AsArray().at(0).AsDouble(),
                                              map_render.at("stop_label_offset").AsArray().at(1).AsDouble()},
                                             underlayer_color, map_render.at("underlayer_width").AsDouble(),
                                             color_palette_variant};
    return render_settings;
}

std::filesystem::path JsonReader::GetOutputFilePath() const {
    const json::Node& root_node = document_.GetRoot();
    if (!(root_node.IsMap() &&
          root_node.AsMap().count("serialization_settings"s) &&
          root_node.AsMap().at("serialization_settings"s).IsMap() &&
          root_node.AsMap().at("serialization_settings"s).AsMap().count("file"s) &&
          root_node.AsMap().at("serialization_settings"s).AsMap().at("file"s).IsString() &&
         !root_node.AsMap().at("serialization_settings"s).AsMap().at("file"s).AsString().empty())) {
        throw std::logic_error("Json document is not count \"serialization_settings\".");
    }
    
    const auto& file_path = root_node.AsMap().at("serialization_settings"s).AsMap().at("file"s).AsString();
    
    return {file_path};
}

std::filesystem::path JsonReader::GetInputFilePath() const {
    return GetOutputFilePath();
}

}  // namespace TransportGuide::IoRequests