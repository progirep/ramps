#include "mdp.hpp"
#include <map>
#include <set>
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
#include <list>

// TypeDefs
typedef std::set<unsigned int> StateSetType;


// Value Iteration
std::vector<std::pair<double,unsigned int> > MDP::valueIteration(const std::map<unsigned int, double> &fixedValues) const {

    // Initialize result
    std::vector<std::pair<double,unsigned int> > result(states.size());
    std::vector<bool> touchable(states.size());
    for (unsigned int i=0;i<states.size();i++) {
        result[i] = std::pair<double,unsigned int>(0.0,0);
        touchable[i] = true;
    }
    for (auto &a : fixedValues) {
        result[a.first].first = a.second;
        touchable[a.first] = false;
    }

    // Perform iteration
    double diff = 1;
    //std::cerr << "vi";
    while (diff > std::numeric_limits<double>::min()*128) {
        // std::cerr << ",";
        diff = 0.0;
        for (unsigned int i=0;i<states.size();i++) {
            unsigned int dir = 0;
            if (touchable[i]) {
                double bestValue = 0.0;
                for (unsigned int j=0;j<transitions[i].size();j++) {
                    auto &a = transitions[i][j];
                    double newValue = 0.0;
                    for (auto &e : a.edges) {
                        newValue += e.first*result[e.second].first;
                    }
                    if (newValue > bestValue) {
                        bestValue = newValue;
                        dir = j;
                    }
                }
                diff += std::abs(bestValue - result[i].first);
                result[i] = std::pair<double,unsigned int>(bestValue,dir);
            }
            assert(result[i].second < transitions[i].size());
        }
    }

    // Now recompute all fixed-probability values
    for (unsigned int i=0;i<states.size();i++) {
        if (!(touchable[i])) {
            double bestValue = 0.0;
            unsigned int dir = 0;
            for (unsigned int j=0;j<transitions[i].size();j++) {
                auto &a = transitions[i][j];
                double newValue = 0.0;
                for (auto &e : a.edges) {
                    if (fixedValues.count(e.second)>0) {
                        newValue += e.first*fixedValues.at(e.second);
                    } else {
                        newValue += e.first*result[e.second].first;
                    }
                }
                if (newValue > bestValue) {
                    bestValue = newValue;
                    dir = j;
                }
            }
            result[i] = std::pair<double,unsigned int>(bestValue,dir);
        }
        assert(result[i].second < transitions[i].size());
    }

    return result;

}



std::map<StrategyTransitionPredecessor,StrategyTransitionChoice> ParityMDP::computeRAPolicy(double raLevel) const {

    // The final strategy
    std::map<StrategyTransitionPredecessor,StrategyTransitionChoice> strategy;
    unsigned int strategyMemoryUsedSoFar = 0;

    // Outer Loop: Iterate over the number of possible switchbacks
    unsigned int nofTargetColorSwitchbacks = 0;
    StateSetType winningOuterGoalStates;
    unsigned int oldNofWinningOuterGoalStates;
    do {
        std::cerr << "Outer iteration!\n";
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

            // 3. Prepare the MDP for Value iteration
            //    This is a special MDP in which each state
            //    is copied: whenever an odd color > currentColor is
            //    visited, the run moves to the second copy. The
            //    second copy starts in state "states.size()" (using
            //    the numbers from the actual product MDP).
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

            // 2. Perform the fixpoint operation
            std::vector<std::pair<double,unsigned int> > values; // The positional final policy
            unsigned int oldNofInnerGoalStates = (unsigned int)-1;
            while (oldNofInnerGoalStates != currentGoalStates.size()) {
                oldNofInnerGoalStates = currentGoalStates.size();

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
                values = mdpForAnalysis.valueIteration(fixedValues);
                assert(values.size()==states.size()*2);

                // Debugging: Print
                std::cerr << "Results of value iteration for minGoalColor:" << minGoalColor << std::endl;
                for (unsigned int i=0;i<states.size();i++) {
                    std::cerr << "- " << states[i].label << ": \t" << values[i].first << " by trans " << values[i].second << "\n";
                }

                // Update set of goal state that are reachable under the raLevel
                for (auto it = currentGoalStates.begin(); it != currentGoalStates.end();){
                    if (values[*it].first<raLevel)
                        currentGoalStates.erase(it++);
                    else
                        ++it;
                }
            }

            // Update the strategy
            {
                std::list<unsigned int> todoNonBackup; // States in the mdpForAnalysis
                std::set<unsigned int> doneNonBackup;

                // Fill todo list
                for (auto it = currentGoalStates.begin(); it != currentGoalStates.end();it++ ){
                    auto key = StrategyTransitionPredecessor(*it,0);
                    if (strategy.count(key)==0) {
                        todoNonBackup.push_back(*it);
                        doneNonBackup.insert(*it);
                    }
                }

                // Add new parts to the strategy: First, the non-backup motion
                std::list<unsigned int> todoBackup;
                std::set<unsigned int> doneBackup;
                strategyMemoryUsedSoFar++;
                while (todoNonBackup.size()>0) {

                    std::cerr << "Non-Backup!\n";

                    unsigned int thisOne = todoNonBackup.front();
                    todoNonBackup.pop_front();

                    unsigned int srcData = currentGoalStates.count(thisOne)>0?0:strategyMemoryUsedSoFar;
                    unsigned int chosenTransition = values[thisOne].second;
                    std::cerr << "ChosenTransition: " << chosenTransition << std::endl;
                    std::map<unsigned int, unsigned int> newData;
                    if (values[thisOne].first!=0.0) { // Exact comparison with 0.0 is OK here.
                        for (auto &e : mdpForAnalysis.transitions[thisOne].at(chosenTransition).edges) {
                            unsigned int dest = e.second;
                            std::cerr << "ISGOALSTATE: " << currentGoalStates.count(dest) << std::endl;
                            if (currentGoalStates.count(dest)>0) {
                                newData[dest] = 0;
                            } else if (dest >= states.size()) {
                                // Backup
                                newData[dest % states.size()] = strategyMemoryUsedSoFar +1;
                                if (doneBackup.count(dest)==0) {
                                    todoBackup.push_back(dest);
                                    doneBackup.insert(dest);
                                }
                            } else {
                                newData[dest] = strategyMemoryUsedSoFar;
                                std::cerr << "Yay!\n";
                                if (doneNonBackup.count(dest)==0) {
                                    todoNonBackup.push_back(dest);
                                    doneNonBackup.insert(dest);
                                }
                            }
                        }
                        std::cerr << "Setting Strategy transitions for " << thisOne << " " << srcData << " non-backup.\n";
                        strategy[StrategyTransitionPredecessor(thisOne,srcData)] = StrategyTransitionChoice(chosenTransition,newData);
                    }
                }

                // Add new parts to the strategy: Now, the backup motion
                strategyMemoryUsedSoFar++;
                while (todoBackup.size()>0) {

                    unsigned int thisOne = todoBackup.front();
                    todoBackup.pop_front();

                    if (values[thisOne].first!=0.0) { // Exact comparison with 0.0 is OK here.
                        unsigned int chosenTransition = values[thisOne].second;
                        std::map<unsigned int, unsigned int> newData;
                        for (auto &e : mdpForAnalysis.transitions[thisOne][chosenTransition].edges) {
                            unsigned int dest = e.second;
                            if (currentGoalStates.count(dest % states.size())>0) {
                                newData[dest % states.size()] = 0;
                            } else if (dest >= states.size()) {
                                // Backup
                                newData[dest % states.size()] = strategyMemoryUsedSoFar;
                                if (doneBackup.count(dest)==0) {
                                    todoBackup.push_back(dest);
                                    doneBackup.insert(dest);
                                }
                            } else {
                                throw "Internal error in the MDP-for-Analysis";
                            }
                        }
                        std::cerr << "Setting Strategy transitions for " << thisOne % states.size() << " " << strategyMemoryUsedSoFar << " backup.\n";
                        strategy[StrategyTransitionPredecessor(thisOne % states.size(),strategyMemoryUsedSoFar)] = StrategyTransitionChoice(chosenTransition,newData);
                    }
                }
            }

            // Add all newly found goal states.
            winningOuterGoalStates.insert(currentGoalStates.begin(),currentGoalStates.end());
        }

        nofTargetColorSwitchbacks++;
    } while (winningOuterGoalStates.size()!=oldNofWinningOuterGoalStates);

    // Compute strategy towards the goal states
    MDP mdpForAnalysis;
    mdpForAnalysis.actions = actions;
    mdpForAnalysis.initialState = initialState;
    mdpForAnalysis.states = states;
    mdpForAnalysis.transitions = transitions;
    std::map<unsigned, double> fixedValues;
    for (auto a : winningOuterGoalStates) {
        fixedValues[a] = 1.0;
    }
    /*std::vector<std::pair<double,unsigned int> > values = mdpForAnalysis.valueIteration(fixedValues);
    for (unsigned int i=0;i<states.size();i++) {
        if (values[i].first>=raLevel) {
            auto key = StrategyTransitionPredecessor(i,0);
            if (strategy.count(key)==0) {
                strategy[key] = StrategyTransitionChoice(values[i].second);
                // TODO: Add Memory update...
            }
        }
    }*/

    return strategy;
}


void ParityMDP::printPolicy(const std::map<StrategyTransitionPredecessor,StrategyTransitionChoice> &policy) {
    std::cout << policy.size() << "\n";
    for (auto &entry : policy) {
        std::cout << entry.first.mdpState << " " << entry.first.dataState << " " << entry.second.action << "\n";
        for (auto &entry2 : entry.second.memoryUpdate) {
            std::cout << "-> " << entry2.first << " " << entry2.second << "\n";
        }
    }
}
