#ifndef __MDP_HPP____
#define __MDP_HPP____

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

struct MDPState {
    std::vector<std::string> label;
    MDPState(std::vector<std::string> _label) : label(_label) {}
};

struct MDPTransition {
    int action;
    std::vector<std::pair<double, unsigned int> > edges;
    MDPTransition() : action(-1) {}
};

struct MDP {
    std::vector<std::string> actions;
    std::vector<std::string> labelComponents;
    std::vector<MDPState> states;
    std::vector<std::vector<MDPTransition> > transitions;
    unsigned int initialState; // is (unsigned int)-1 if undefined

    MDP() : initialState(-1) {}
    MDP(std::string baseFilename);

    std::vector<std::pair<double,unsigned int> > valueIteration(const std::map<unsigned int, double> &fixedValues, double epsilon, bool computePolicyEagerly) const;

};


/**
 * @brief This struct captures the precondition in a transition in the generated strategy. It is basically the look-up type for the map implementing the strategy
 */
struct StrategyTransitionPredecessor{
    unsigned int mdpState;
    unsigned int dataState;
    StrategyTransitionPredecessor(unsigned int _mdpState,unsigned int _dataState) : mdpState(_mdpState), dataState(_dataState) {}

    bool operator<(const StrategyTransitionPredecessor &other) const {
        int v1 = ((int)mdpState)-((int)other.mdpState);
        if (v1<0) return true;
        if (v1>0) return false;
        return ((int)dataState)-((int)other.dataState)<0;
    }

    bool operator==(const StrategyTransitionPredecessor &other) const {
        return (other.mdpState==mdpState) && (other.dataState==dataState);
    }
};

struct StrategyTransitionPredecessorHash {
    inline std::size_t operator()(const StrategyTransitionPredecessor& k) const {
        return (size_t)(k.mdpState)*1337 ^ (size_t)(k.dataState*23);
    }
};

struct StrategyTransitionChoice {
    unsigned int action;
    std::map<unsigned int /* mdpstate */, unsigned int /* dataState */> memoryUpdate;
    StrategyTransitionChoice() : action((unsigned int)-1) {}
    StrategyTransitionChoice(unsigned int _action) : action(_action) {}
    StrategyTransitionChoice(unsigned int _action, const std::map<unsigned int /* mdpstate */, unsigned int /* dataState */> &_memoryUpdate) : action(_action), memoryUpdate(_memoryUpdate) {}
};


struct ParityMDP {
private:
    std::vector<std::string> actions;
    std::vector<MDPState> states;
    std::vector<std::vector<MDPTransition> > transitions;
    std::vector<unsigned int> colors;
    std::map<unsigned int,unsigned int> toNonParityMDPMapper;
    unsigned int initialState; // is always 0
    unsigned int nofColors;



public:
    ParityMDP(std::string parityFilename, const MDP &baseMDP);
    void dumpDot(std::ostream &output) const;
    std::pair<std::unordered_map<StrategyTransitionPredecessor,StrategyTransitionChoice,StrategyTransitionPredecessorHash>,double> computeRAPolicy(double raLevel, double epsilon, bool computePolicyEagerly) const;
    void printPolicy(const std::unordered_map<StrategyTransitionPredecessor,StrategyTransitionChoice,StrategyTransitionPredecessorHash> &policy) const;
};


#endif
