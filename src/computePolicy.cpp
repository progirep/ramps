#include "mdp.hpp"
#include <map>
#include <set>
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>

/**
 * @brief This struct captures the precondition in a transition in the generated strategy. It is basically the look-up type for the map implementing the strategy
 */
struct StrategyTransitionPredecessor{
    unsigned int mdpState;
    unsigned int dataState;
    StrategyTransitionPredecessor(unsigned int _mdpState,unsigned int _dataState) : mdpState(_mdpState), dataState(_dataState) {}
};

struct StrategyTransitionChoice {
    unsigned int action;
    unsigned int dataState;
    StrategyTransitionChoice(unsigned int _action,unsigned int _dataState) : action(_action), dataState(_dataState) {}
};

// TypeDefs
typedef std::set<unsigned int> StateSetType;


// Value Iteration
std::vector<double> MDP::valueIteration(const std::map<unsigned int, double> &fixedValues) const {

    // Initialize result
    std::vector<double> result(states.size());
    std::vector<bool> touchable(states.size());
    for (unsigned int i=0;i<states.size();i++) {
        result[i] = 0.0;
        touchable[i] = true;
    }
    for (auto &a : fixedValues) {
        result[a.first] = a.second;
        touchable[a.first] = false;
    }

    // Perform iteration
    double diff = 1;
    //std::cerr << "vi";
    while (diff > std::numeric_limits<double>::min()*128) {
        // std::cerr << ",";
        diff = 0.0;
        for (unsigned int i=0;i<states.size();i++) {
            if (touchable[i]) {
                double bestValue = 0.0;
                for (auto &a : transitions[i]) {
                    double newValue = 0.0;
                    for (auto &e : a.edges) {
                        newValue += e.first*result[e.second];
                    }
                    bestValue = std::max(bestValue,newValue);
                }
                diff += std::abs(bestValue - result[i]);
                result[i] = bestValue;
            }
        }
    }

    // Now recompute all fixed-probability values
    for (unsigned int i=0;i<states.size();i++) {
        if (!(touchable[i])) {
            double bestValue = 0.0;
            for (auto &a : transitions[i]) {
                double newValue = 0.0;
                for (auto &e : a.edges) {
                    if (fixedValues.count(e.second)>0) {
                        newValue += e.first*fixedValues.at(e.second);
                    } else {
                        newValue += e.first*result[e.second];
                    }
                }
                bestValue = std::max(bestValue,newValue);
            }
            result[i] = bestValue;
        }
    }

    return result;

}



void ParityMDP::computeRAPolicy(double raLevel) const {

    // std::map<StrategyTransitionPredecessor,StrategyTransitionChoice> strategy;

    // Outer Loop: Iterate over the number of possible switchbacks
    unsigned int nofTargetColorSwitchbacks = 0;
    StateSetType winningOuterGoalStates;
    unsigned int oldNofWinningOuterGoalStates;
    do {
        oldNofWinningOuterGoalStates = winningOuterGoalStates.size();

        // Inner Loop: Iterate over the possible goal colors
        // --> in the following for line, use a slight trick to remove the need for signed numbers
        for (unsigned int minGoalColor=nofColors & (-2);minGoalColor<=nofColors;minGoalColor-=2) {

            // Greatest fix-point over the goal states:
            // 1. Build current set of goal states
            StateSetType currentGoalStates;
            for (unsigned int i=0;i<states.size();i++) {
                unsigned int currentColor = colors[i];
                if (((currentColor & 1)==0) && (currentColor>=minGoalColor)) {
                    currentGoalStates.insert(i);
                }
            }

            // 2. Perform the fixpoint operation
            unsigned int oldNofInnerGoalStates = (unsigned int)-1;
            while (oldNofInnerGoalStates != currentGoalStates.size()) {
                oldNofInnerGoalStates = currentGoalStates.size();

                // 3. Prepare the MDP for Value iteration
                //    This is a special MDP in which each state
                //    is copied: whenever an odd color > currentColor is
                //    visited, the run moves to the second copy.
                //
                //    We need this extra analysis as when such a color
                //    is visited in the middle between the goal states,
                //    the probabilities still need to add up to raLevel,
                //    and otherwise the computation would not be right.
                MDP mdpForAnalysis;
                mdpForAnalysis.actions = actions;
                mdpForAnalysis.initialState = initialState;
                // ---> First copy of every state
                mdpForAnalysis.states = states;
                // ---> Second copy of every state
                for (auto &s : states) {
                    mdpForAnalysis.states.push_back(MDPState(s.label+"'"));
                }
                // ---> First copy of every transition:
                mdpForAnalysis.transitions.resize(mdpForAnalysis.states.size());
                for (unsigned int i=0;i<transitions.size();i++) {
                    for (unsigned int j=0;j<transitions[i].size();j++) {
                        MDPTransition newTransA;
                        MDPTransition newTransB;
                        newTransA.action = transitions[i][j].action;
                        newTransB.action = transitions[i][j].action;
                        for (auto edge : transitions[i][j].edges) {
                            unsigned int currentColor = colors[edge.second];
                            if (((currentColor & 1)>0) && (currentColor>minGoalColor)) {
                                newTransA.edges.push_back(std::pair<double,unsigned int>(edge.first,edge.second + states.size()));
                            } else {
                                newTransA.edges.push_back(edge);
                            }
                            newTransB.edges.push_back(std::pair<double,unsigned int>(edge.first,edge.second + states.size()));
                        }
                        mdpForAnalysis.transitions[i].push_back(newTransA);
                        mdpForAnalysis.transitions[i+states.size()].push_back(newTransB);
                    }
                }

                // 2. Prepare the fixed values for value iteration
                std::map<unsigned, double> fixedValues;
                for (unsigned int i=0;i<states.size();i++) {
                    unsigned int currentColor = colors[i];
                    if (((currentColor & 1)>0) && (currentColor>minGoalColor)) {
                        fixedValues[i] = 0.0;
                    }
                }
                // -> Goal states are the innermost goal states....
                for (auto gs : currentGoalStates) {
                    fixedValues[gs] = 1.0;
                }
                // ... and the ones found earlier...
                for (auto gs : winningOuterGoalStates) {
                    fixedValues[gs] = 1.0;
                    fixedValues[gs+states.size()] = 1.0;
                }

                // 3. Perform Value iteration
                std::vector<double> values = mdpForAnalysis.valueIteration(fixedValues);
                assert(values.size()==states.size()*2);

                // Debugging: Print
                std::cerr << "Results of value iteration for minGoalColor:" << minGoalColor << std::endl;
                for (unsigned int i=0;i<states.size();i++) {
                    std::cerr << "- " << states[i].label << ": \t" << values[i] << "\n";
                }

                // Update set of goal state that are reachable under the raLevel
                for (auto it = currentGoalStates.begin(); it != currentGoalStates.end(); ){
                    if (values[*it]<raLevel)
                        currentGoalStates.erase(it++);
                    else
                        ++it;
                }
            }

            // Add all newly found goal states.
            winningOuterGoalStates.insert(currentGoalStates.begin(),currentGoalStates.end());
        }


        nofTargetColorSwitchbacks++;
    } while (winningOuterGoalStates.size()!=oldNofWinningOuterGoalStates);




}
