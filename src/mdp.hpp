#ifndef __MDP_HPP____
#define __MDP_HPP____

#include <string>
#include <vector>
#include <map>

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

    MDP() : initialState(-1) {}
    MDP(std::string baseFilename);

    std::vector<double> valueIteration(const std::map<unsigned int, double> &fixedValues) const;

};

struct ParityMDP {
private:
    std::vector<std::string> actions;
    std::vector<MDPState> states;
    std::vector<std::vector<MDPTransition> > transitions;
    std::vector<unsigned int> colors;
    unsigned int initialState; // is always 0
    unsigned int nofColors;



public:
    ParityMDP(std::string parityFilename, const MDP &baseMDP);
    void dumpDot(std::ostream &output) const;
    void computeRAPolicy(double raLevel) const;

};


#endif
