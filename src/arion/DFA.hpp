#ifndef ARION_DFA_H
#define ARION_DFA_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace arion {
    class DFA {
    public:
        // Constant(s)
        const int INVALID_STATE = -1;

        // Constructor
        DFA();

        // Current state
        int getCurrentState() const;
        bool isCurrentStateInvalid() const;
        void resetToStartState();
        
        // Start state
        int getStartState() const;
        void setStartState(int stateId);

        // State
        bool hasState(int stateId) const;
        std::string getStateName(int stateId);
        void addState(int stateId, std::string name);
        void removeState(int stateId);

        // Final State
        bool isFinalState(int stateId) const;
        void setFinalState(int stateId, bool isFinalState);

        // Transition
        bool transition(char input);
        bool canTransition(char input);
        bool canTransition(int from, char input);
        void addTransition(int from, char input, int to);
        void addTransition(int from, std::string inputs, int to);
        void removeTransition(int from, char input);
        void removeTransition(int from, std::string inputs);

    private:
        struct hash {
            std::size_t operator()(const std::pair<int, char> &p) const
            {
                return ((long long)p.first << 8) ^ p.second;
            }
        };
        int currentState_ = 0;
        int startState_ = 0;
        std::unordered_map<int, std::string> states_;
        std::unordered_set<int> finalStates_;
        std::unordered_map<std::pair<int, char>, int, DFA::hash> transitionMap_;
    };
}

#endif