#pragma once

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

/*
#include "../business_logic/transport_catalogue.h"
#include "../domain/domain.h"
#include "../external/svg.h"
#include "map_renderer.h"

namespace TransportGuide::IoRequests {

class RequestHandler {
public:
    RequestHandler(const BusinessLogic::TransportCatalogue& catalogue, const renderer::MapRenderer& renderer, IoMethod io_method = IoMethod::JSON);
    
    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<Domain::BusInfo> GetBusInfo(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через остановку
    const std::vector<const Domain::Bus*> GetBusesByStop(const std::string_view& stop_name) const;

    svg::Document RenderMap() const;

private:
    const BusinessLogic::TransportCatalogue& catalogue_;
    const renderer::MapRenderer& renderer_;
};
}*/
