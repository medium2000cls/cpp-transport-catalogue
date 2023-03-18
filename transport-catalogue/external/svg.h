#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <variant>

namespace TransportGuide::svg {
using namespace std::literals;
class Object;


struct Rgb {
    Rgb() = default;
    Rgb(uint8_t red, uint8_t green, uint8_t blue);
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};


struct Rgba {
    Rgba() = default;
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity);
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.;
};


using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"};

struct RenderColorAttribute {
    std::string operator() (std::monostate) const;
    std::string operator() (std::string str) const;
    std::string operator() (Rgb rgb) const;
    std::string operator() (Rgba rgba) const;
};

std::ostream& operator<< (std::ostream& out, const Color& color);


enum class StrokeLineCap {
    BUTT, ROUND, SQUARE
};

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stroke_line_cap);

enum class StrokeLineJoin {
    ARCS, BEVEL, MITER, MITER_CLIP, ROUND
};

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_line_join);


template<typename T>
class PathProps {
public:
    /**Установить цвет заливки*/
    T& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    
    /**Установить цвет обводки*/
    T& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    
    /**Установить ширину обводки*/
    T& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    
    /**Установить тип торца*/
    T& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }
    
    /**Установить тип соединения*/
    T& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;
    
    void RenderAttrs(std::ostream& out) const {
        if (!std::holds_alternative<std::monostate>(fill_color_)) {
            out << " fill=\""sv << std::visit(RenderColorAttribute {}, fill_color_) << "\""sv;
        }
        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
            out << " stroke=\""sv << std::visit(RenderColorAttribute {}, stroke_color_) << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    T& AsOwner() {
        return static_cast<T&>(*this);
    }
    
    Color fill_color_ = std::monostate {};
    Color stroke_color_ = std::monostate {};
    std::optional<double> stroke_width_ = std::nullopt;
    std::optional<StrokeLineCap> stroke_line_cap_ = std::nullopt;
    std::optional<StrokeLineJoin> stroke_line_join_ = std::nullopt;
};


struct ObjectContainer {
    template<typename ObjectType>
    void Add(ObjectType object) {
        AddPtr(std::make_unique<ObjectType>(std::move(object)));
    }
    
    /** Добавляет объект-наследник svg::Object*/
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};


struct Drawable {
    virtual void Draw(svg::ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};


struct Point {
    Point() = default;
    
    Point(double x, double y) : x(x), y(y) {
    }
    
    double x = 0;
    double y = 0;
};


/**
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out) : out(out) {
    }
    
    RenderContext(std::ostream& out, int indent_step, int indent = 0) : out(out), indent_step(indent_step), indent(
            indent) {
    }
    
    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }
    
    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }
    
    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};


/**
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class   Object {
public:
    void Render(const RenderContext& context) const;
    
    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};


/**
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;
    
    Point center_ = {0, 0};
    double radius_ = 1.0;
};


/**
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> pointers_;
};


/**
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);
    
    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);
    
    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);
    
    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);
    
    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);
    
    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;
    
    Point pos_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};


class Document : public ObjectContainer {
    std::vector<std::unique_ptr<Object>> objects_;

public:
    /** Добавляет в svg-документ объект-наследник svg::Object*/
    void AddPtr(std::unique_ptr<Object>&& obj) override;
    
    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;
};

}  // namespace svg