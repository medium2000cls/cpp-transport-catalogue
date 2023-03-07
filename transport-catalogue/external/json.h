#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace TransportGuide::json {

class Node;
using Array = std::vector<Node>;
using Dict = std::map<std::string, Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

using NodeVariant = std::variant<std::nullptr_t, bool, int, double, std::string, Array, Dict>;

class Node final : private NodeVariant {
public:
    
    using variant::variant;
    
    [[nodiscard]] bool IsInt() const;
    [[nodiscard]] bool IsDouble() const; //Возвращает true, если в Node хранится int либо double.
    [[nodiscard]] bool IsPureDouble() const; //Возвращает true, если в Node хранится double.
    [[nodiscard]] bool IsBool() const;
    [[nodiscard]] bool IsString() const;
    [[nodiscard]] bool IsNull() const;
    
    [[nodiscard]] bool IsArray() const;
    [[nodiscard]] bool IsMap() const; //Тот же Dict
    
    
    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const; //Возвращает значение типа double, если внутри хранится double либо int.
    // В последнем случае возвращается приведённое в double значение.
    const std::string& AsString() const;
    
    const Array& AsArray() const;
    const Dict& AsMap() const; //Тот же Dict
    
    bool operator==(const Node& rhs) const;
    bool operator!=(const Node& rhs) const;
    
    template <typename T>
    bool operator== (const T&& rhs) {
        if (const auto* value_ptr = std::get_if<T>(*this)) {
            return *value_ptr == rhs;
        }
        return false;
    }
};


class Document {
public:
    explicit Document(Node root);
    
    const Node& GetRoot() const;
    
    bool operator==(const Document& rhs) const;
    bool operator!=(const Document& rhs) const;
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json