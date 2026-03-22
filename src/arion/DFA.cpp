#include "DFA.hpp"
#include <stdexcept>
using namespace arion;

// Constructor
DFA::DFA()
{
    addState(-1, "invalid");
}

// Current state
int DFA::getCurrentState() const
{
    return currentState_;
}

bool DFA::isCurrentStateInvalid() const
{
    return currentState_ == INVALID_STATE;
}

// Start state
int DFA::getStartState() const
{
    return startState_;
}
void DFA::setStartState(int stateId)
{
    if (!hasState(stateId)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateId));
    }
    startState_ = stateId;
}
void DFA::resetToStartState()
{
    currentState_ = startState_;
}

// State
bool DFA::hasState(int stateId) const
{
    return states_.count(stateId);
}
std::string DFA::getStateName(int stateId)
{
    if (!hasState(stateId)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateId));
    }
    return states_[stateId];
}
void DFA::addState(int stateId, std::string name)
{
    if (stateId < 0) {
        throw std::invalid_argument("State id must not be negative");
    }
    states_[stateId] = std::move(name);
}
void DFA::removeState(int stateId)
{
    states_.erase(stateId);
}

// Final State
bool DFA::isFinalState(int stateId) const
{
    if (!hasState(stateId)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateId));
    }
    return finalStates_.count(stateId);
}
void DFA::setFinalState(int stateId, bool isFinalState)
{
    if (!hasState(stateId)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateId));
    }
    if (isFinalState) {
        finalStates_.insert(stateId);
    }
    else {
        finalStates_.erase(stateId);
    }
}

// Transition
bool DFA::transition(char input)
{
    if (!canTransition(input)) {
        currentState_ = INVALID_STATE;
        return false;
    }
    else {
        currentState_ = transitionMap_[{currentState_, input}];
        return true;
    }
}
bool DFA::canTransition(char input)
{
    return transitionMap_.count({currentState_, input});
}
bool DFA::canTransition(int from, char input)
{
    if (!hasState(from)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(from));
    }
    return transitionMap_.count({from, input});
}
void DFA::addTransition(int from, char input, int to)
{
    if (!hasState(from)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(from));
    }
    if (!hasState(to)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(to));
    }
    if (canTransition(from, input)) {
        throw std::invalid_argument("DFA can't have multiple same transition to different state with " +
                                    std::to_string(input) + " from " + getStateName(from));
    }
    transitionMap_[{from, input}] = to;
}
void DFA::addTransition(int from, std::string inputs, int to)
{
    if (!hasState(from)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(from));
    }
    if (!hasState(to)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(to));
    }
    for (char input : inputs) {
        addTransition(from, input, to);
    }
}
void DFA::removeTransition(int from, char input)
{
    if (!hasState(from)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(from));
    }
    transitionMap_.erase({from, input});
}
void DFA::removeTransition(int from, std::string inputs)
{
    if (!hasState(from)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(from));
    }
    for (char input : inputs) {
        transitionMap_.erase({from, input});
    }
}