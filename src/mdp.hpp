#ifndef __MDP_HPP____
#define __MDP_HPP____

#include <string>
#include <vector>

struct MDPState {
    std::string label;
    MDPState(std::string _label) : label(_label) {}
};

struct MDPTransition {
    int action;
    std::vector<std::pair<double, unsigned int> > edges;
    MDPTransition() : action(-1) {}
};

struct MDP {
    std::vector<std::string> actions;
    std::vector<MDPState> states;
    std::vector<std::vector<MDPTransition> > transitions;
    unsigned int initialState; // is (unsigned int)-1 if undefined
    MDP(std::string baseFilename);
};

struct ParityMDP {
    std::vector<std::string> actions;
    std::vector<MDPState> states;
    std::vector<std::vector<MDPTransition> > transitions;
    std::vector<unsigned int> colors;
    unsigned int initialState; // is always 0
    unsigned int nofColors;
    ParityMDP(std::string parityFilename, const MDP &baseMDP);

};


#endif
