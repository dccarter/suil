//
// Created by Mpho Mbotho on 2021-06-16.
//

#include "tp.hpp"

#include <suil/base/json.hpp>

namespace suil::saw {

    UnorderedMap<IntKeyProcessor::VerbHandler> IntKeyProcessor::sHandlers = {
        {"set", &IntKeyProcessor::setKey},
        {"dec", &IntKeyProcessor::decrementKey},
        {"inc", &IntKeyProcessor::incrementKey}
    };

    void IntKeyProcessor::apply()
    {
        auto& txn = Ego.getTransaction();
        idebug("applying transaction");

        auto obj = json::Object::decode(txn.payload());

        auto verb = getString("Verb", obj);
        auto handler = sHandlers.find(verb);
        if (handler == sHandlers.end()) {
            throw InvalidTransaction("'", verb, "' is not a supported verb");
        }

        (this->* handler->second)(obj);
    }

    String IntKeyProcessor::getString(const String& name, json::Object& obj)
    {
        auto value = (String) obj.get(name, false);
        if (value.empty()) {
            throw InvalidTransaction(name, " required");
        }
        return value;
    }

    std::uint32_t IntKeyProcessor::getNumber(const String& name, json::Object& obj)
    {
        auto value = obj.get(name, false);
        if (!value) {
            throw InvalidTransaction(name, " required");
        }

        return (std::uint32_t) value;
    }

    void IntKeyProcessor::setKey(json::Object& obj)
    {
        auto name = getString("Name", obj);
        auto value = getNumber("Value", obj);
        auto addr = mAe(name);
        idebug("setKey {name: " PRIs ", value: %u}", _PRIs(name), value);

        auto state = Ego.globalState().getState(addr);
        json::Object current;
        if (!state.empty()) {
            current = json::Object::decode(state);
            auto stateValue = current.get(name, false);
            if (stateValue) {
                throw InvalidTransaction(
                        "Invalid 'Set' arguments, already exists {name: ",
                        name, ", value: ", (std::uint32_t) stateValue, "}");
            }
        }
        else {
            current = json::Object(json::Obj, name(), value);
        }
        auto str = json::encode(current);
        Ego.globalState().setState(addr, fromStdString(str));
    }

    void IntKeyProcessor::decrementKey(json::Object& obj)
    {
        auto name = getString("Name", obj);
        auto value = getNumber("Value", obj);
        auto addr = mAe(name);
        idebug("decrementKey {name: " PRIs ", value: %u}", _PRIs(name), value);

        json::Object current;
        auto state = Ego.globalState().getState(addr);
        if (state.empty()) {
           throw InvalidTransaction("Failed to decrement, '", name, "' does not have a state");
        }

        current = json::Object::decode(state);
        if (!current.get(name, false)) {
            throw InvalidTransaction("Failed to decrement, '", name, "' does not exist in state");
        }

        auto stateValue = Ego.getNumber(name, current);
        stateValue -= value;
        current = json::Object(json::Obj, name(), stateValue);
        auto str = json::encode(current);
        Ego.globalState().setState(addr, fromStdString(str));
    }

    void IntKeyProcessor::incrementKey(json::Object& obj)
    {
        auto name = getString("Name", obj);
        auto value = getNumber("Value", obj);
        auto addr = mAe(name);
        idebug("incrementKey {name: " PRIs ", value: %u}", _PRIs(name), value);

        json::Object current;
        auto state = Ego.globalState().getState(addr);
        if (state.empty()) {
            throw InvalidTransaction("Failed to increment, '", name, "' does not have a state");
        }

        current = json::Object::decode(state);
        if (!current.get(name, false)) {
            throw InvalidTransaction("Failed to increment, '", name, "' does not exist in state");
        }

        auto stateValue = Ego.getNumber(name, current);
        stateValue += value;
        current = json::Object(json::Obj, name(), stateValue);
        auto str = json::encode(current);
        Ego.globalState().setState(addr, fromStdString(str));
    }

    typename ProcessorBase::Ptr IntKeyHandler::getProcessor(Transaction&& txn, GlobalState&& state)
    {
        return std::make_shared<IntKeyProcessor>(addressEncoder(), std::move(txn), std::move(state));
    }
}