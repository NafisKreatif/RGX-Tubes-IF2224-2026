#include <iostream>
#include <unordered_map>
#include <unordered_set>

class DFA {
private:
    int current_state = 0;
    int start_state = 0;
    std::unordered_map<int, std::string> states;
    std::unordered_set<int> final_states;
    std::unordered_map<std::pair<int, char>, int> transition_map;

public:
    const int INVALID_STATE = -1;

    // Current state
    int getCurrentState() const;

    // Start state
    int getStartState() const;
    void setStartState(int stateIndex);

    // State
    bool hasState(int stateIndex) const;
    std::string getStateName(int stateIndex);
    void addState(int stateIndex, std::string name);
    void removeState(int stateIndex);

    // Final State
    bool isFinalState(int stateIndex) const;
    void setFinalState(int stateIndex, bool isFinalState);

    // Transition
    bool transition(char input);
    bool canTransition(char input);
    bool hasTransition(int from, char input, int to);
    void addTransition(int from, char input, int to);
    void addTransition(int from, std::string inputs, int to);
    void removeTransition(int from, char input);
    void removeTransition(int from, std::string inputs);
};
