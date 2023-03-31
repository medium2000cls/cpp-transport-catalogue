#pragma once
#include "json.h"
#include <string>
#include <memory>
#include <optional>

namespace TransportGuide::json {
using namespace std::literals;

class BuilderKeyContext;
class BuilderDictValueContext;
class BuilderArrayValueContext;

class Builder {
public:
    Builder();
    Node Build();
    Builder& Value(Node);
    Builder& If(bool);
    Builder& Else();
    Builder& EndIf();
    BuilderDictValueContext StartDict();
    BuilderArrayValueContext StartArray();
    BuilderKeyContext Key(std::string);
    Builder& EndDict();
    Builder& EndArray();

private:
    enum class BuildActionType {
        BUILD, KEY, VALUE, DICT, ARRAY
    };
    enum class ActionCondition {
        IF_TRUE, IF_FALSE
    };
    struct ActionInfo {
        BuildActionType type;
        json::Node* node_ptr;
    };
    
    Node node_;
    std::vector<ActionInfo> stack_action_info_;
    std::vector<std::pair<json::Node*, ActionCondition>> actions_condition_info_;
};

class BuilderKeyContext {
    Builder& builder_;
public:
    BuilderKeyContext(Builder& builder);
    BuilderDictValueContext Value(Node);
    BuilderDictValueContext StartDict();
    BuilderArrayValueContext StartArray();
};

class BuilderDictValueContext {
    Builder& builder_;
public:
    BuilderDictValueContext(Builder& builder);
    BuilderKeyContext Key(std::string);
    BuilderDictValueContext If(bool);
    BuilderDictValueContext Else();
    BuilderDictValueContext EndIf();
    Builder& EndDict();
};

class BuilderArrayValueContext {
    Builder& builder_;
public:
    BuilderArrayValueContext(Builder& builder);
    BuilderArrayValueContext Value(Node);
    BuilderDictValueContext StartDict();
    BuilderArrayValueContext StartArray();
    BuilderArrayValueContext If(bool);
    BuilderArrayValueContext Else();
    BuilderArrayValueContext EndIf();
    Builder& EndArray();
};
}