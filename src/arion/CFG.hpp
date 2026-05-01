#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace arion {
    // Context Free Grammar
    class CFG {
    public:
        struct Variable {
            int id;
            std::string name;
            std::string toString() const;
        };

        struct Terminal {
            int id;
            int value;
            std::string name;
            std::string toString() const;
        };

        void addVariable(Variable v);
        bool hasVariable(int variableId) const;
        Variable getVariable(int variableId) const;
        void addTerminal(Terminal t);
        bool hasTerminal(int terminalId) const;
        Terminal getTerminal(int terminalId) const;

        void addProduction(int variableId, std::vector<int> product);
        const std::vector<std::vector<int>> &getProductions(int variableId) const;

        int getStartSymbol();
        void setStartSymbol(int symbolType);

    private:
        struct VariableHash {
            std::size_t operator()(const Variable &p) const {
                return p.id;
            }
        };
        struct TerminalHash {
            std::size_t operator()(const Terminal &p) const {
                return p.id;
            }
        };
        std::unordered_set<Variable, VariableHash> variables_;
        std::unordered_set<Terminal, TerminalHash> terminals_;
        std::unordered_map<int, std::vector<std::vector<int>>> productions_; // Variable -> list variable and terminal
        int startSymbol_ = -1;
    };
}