#pragma once

#include "../business_logic/transport_catalogue.h"
#include "json_reader.h"
#include <transport_catalogue.pb.h>

namespace TransportGuide::IoRequests {

class ProtoSerialization : public TransportGuide::IoRequests::ISerializer {
public:
    explicit ProtoSerialization(BusinessLogic::TransportCatalogue& transport_catalogue,
            renderer::MapRenderer& map_renderer);
    
    void Serialize(std::ostream& output) override;
    void Deserialize(std::istream& input) override;

private:
    BusinessLogic::TransportCatalogue& transport_catalogue_;
    renderer::MapRenderer& map_renderer_;
    void SerializerStopCatalog(Serialization::TransportCatalogue& result_catalogue,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue);
    void SerializeerBusCatalog(Serialization::TransportCatalogue& result_catalogue,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue);
    void SerializerCalculatedDistanceCatalog(Serialization::TransportCatalogue& result_catalogue,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue);
    void SerializerRealDistanceCatalog(Serialization::TransportCatalogue& result_catalogue,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue);
    void SerializerRenderSettings(Serialization::TransportCatalogue& result_catalogue);
    void SerializerRoutingSettings(Serialization::TransportRouter& result_user_route_manager,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router);
    void SerializerGraph(Serialization::TransportRouter& result_user_route_manager,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router);
    void SerializerGraphStopToVertexIdCatalog(Serialization::TransportRouter& result_user_route_manager,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router);
    void SerializerGraphEdgeIdToInfoCatalog(Serialization::TransportRouter& result_user_route_manager,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router);
    void SerializerRouter(Serialization::TransportRouter& result_user_route_manager,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router);
    void DeserializerStopCatalog(const Serialization::TransportCatalogue& parsed_catalog,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue,
            std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog);
    void DeserializerBusCatalog(const Serialization::TransportCatalogue& parsed_catalog,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue,
            std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog,
            std::map<uint64_t, const Domain::Bus*>& temp_buses_catalog);
    void DeserializerCalculatedDistanceCatalog(const Serialization::TransportCatalogue& parsed_catalog,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue,
            std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog);
    void DeserializerRealDistanceCatalog(const Serialization::TransportCatalogue& parsed_catalog,
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue,
            std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog);
    BusinessLogic::SerializerTransportRouter ConstructBasicTransportRouter(
            BusinessLogic::SerializerTransportCatalogue& serializer_catalogue);
    void DeserializerRoutingSettings(BusinessLogic::SerializerTransportRouter& serializer_transport_router,
            const Serialization::TransportRouter& parsed_user_route_manager);
    void DeserializerGraph(const Serialization::TransportRouter& parsed_user_route_manager,
            graph::DirectedWeightedGraph<TransportGuide::Domain::TimeMinuts>::SerializerDirectedWeightedGraph& serializer_graph);
    void DeserializerGraphToStopVertexIdCatalog(const std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router,
            const Serialization::TransportRouter& parsed_user_route_manager);
    void DeserializerGraphEdgeIdToInfoCatalog(const std::map<uint64_t, const Domain::Stop*>& temp_stops_catalog,
            const std::map<uint64_t, const Domain::Bus*>& temp_buses_catalog,
            BusinessLogic::SerializerTransportRouter& serializer_transport_router,
            const Serialization::TransportRouter& parsed_user_route_manager);
    void DeserializerRouter(BusinessLogic::SerializerTransportRouter& serializer_transport_router,
            const Serialization::TransportRouter& parsed_user_route_manager);
    void DeserializerRenderSettings(const Serialization::TransportCatalogue& parsed_catalog);
};

}