#include "DFA.hpp"
#include <exception>

// Current state
int DFA::getCurrentState() const
{
    return current_state;
}

// Start state
int DFA::getStartState() const
{
    return start_state;
}
void DFA::setStartState(int stateIndex)
{
    if (!hasState(stateIndex)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateIndex));
    }
    start_state = stateIndex;
}

// State
bool DFA::hasState(int stateIndex) const
{
    return states.count(stateIndex);
}
std::string DFA::getStateName(int stateIndex)
{
    if (!hasState(stateIndex)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateIndex));
    }
    return states[stateIndex];
}
void DFA::addState(int stateIndex, std::string name)
{
    if (!hasState(stateIndex)) {
        states[stateIndex] = name;
    }
}
void DFA::removeState(int stateIndex)
{
    states.erase(stateIndex);
}

// Final State
bool DFA::isFinalState(int stateIndex) const
{
    if (!hasState(stateIndex)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateIndex));
    }
    return final_states.count(stateIndex);
}
void DFA::setFinalState(int stateIndex, bool isFinalState)
{
    if (!hasState(stateIndex)) {
        throw std::invalid_argument("DFA doesn't have state " + std::to_string(stateIndex));
    }
    if (isFinalState) {
        final_states.insert(stateIndex);
    }
    else {
        final_states.erase(stateIndex);
    }
}

// Transition
bool DFA::transition(char input)
{
    if (!canTransition(input)) {
        current_state = INVALID_STATE;
    }
    else {
        current_state = transition_map[{current_state, input}];
    }
}
bool DFA::canTransition(char input)
{
    return transition_map.count({current_state, input});
}
bool DFA::hasTransition(int from, char input, int to)
{
    return transition_map.count({current_state, input}) && transition_map[{from, input}] == to;
}
void DFA::addTransition(int from, char input, int to)
{
    transition_map[{from, input}] = to;
}
void DFA::addTransition(int from, std::string inputs, int to)
{
    for (char input : inputs) {
        transition_map[{from, input}] = to;
    }
}
void DFA::removeTransition(int from, char input)
{
    transition_map.erase({from, input});
}
void DFA::removeTransition(int from, std::string inputs)
{
    for (char input : inputs) {
        transition_map.erase({from, input});
    }
}