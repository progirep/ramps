#include <iostream>
#include "mdp.hpp"


int main(int nofArgs, const char **args) {

    try {

        // Parse parameters
        std::string baseFilename = "";
        for (int i=1;i<nofArgs;i++) {
            if (args[i][0]=='-') {
                // Special parameter
                std::cerr << "Error: Did not understand parameter " << args[i] << std::endl;
                return 1;
            } else {
                if (baseFilename=="") {
                    baseFilename = args[i];
                } else {
                    std::cerr << "Error: More than one file name prefix given.\n";
                    return 1;
                }
            }
        }
        if (baseFilename=="") {
            std::cerr << "Error: No input file name given.\n";
            return 1;
        }

        // Start computation
        const MDP mdp(baseFilename);
        const ParityMDP parityMDP(baseFilename+".parity",mdp);
        //parityMDP.dumpDot(std::cout);
        double quality = 0.001;
        std::pair<std::map<StrategyTransitionPredecessor,StrategyTransitionChoice>,double> bestStrategy;
        while(true) {
            auto thisStrategy = parityMDP.computeRAPolicy(quality);
            std::cerr << "Quality computed: " << thisStrategy.second << std::endl;
            if (thisStrategy.first.size()>0) {
                bestStrategy = thisStrategy;
                quality = bestStrategy.second + 0.0001;
            } else {
                break;
            }
        }
        parityMDP.printPolicy(bestStrategy.first);
        std::cerr << "Quality of the generated strategy: " << bestStrategy.second << std::endl;

    } catch (int error) {
        std::cerr << "Numerical error " << error << std::endl;
        return 1;
    } catch (const char *error) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    } catch (const std::string error) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }

}
