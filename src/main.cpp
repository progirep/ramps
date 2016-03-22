#include <iostream>
#include <sstream>
#include <tuple>
#include <sstream>
#include "mdp.hpp"



int main(int nofArgs, const char **args) {

    try {

        // Parse parameters
        std::string baseFilename = "";
        std::string searchStrategy = "";
        double minQuality = 0.0;
        double maxQuality = 1.0;
        bool computePolicyEagerly = false;

        for (int i=1;i<nofArgs;i++) {
            if (args[i][0]=='-') {
                // Special parameter
                std::string param = args[i];
                if (param=="--ses") {
                    if (nofArgs<i+1) {
                        std::cerr << "Error: No parameter after '--ses'.\n";
                        return 1;
                    }
                    if (searchStrategy!="") {
                        std::cerr << "Error: More than one search strategy given'.\n";
                        return 1;
                    }
                    searchStrategy = args[++i];

                } else if (param=="--min") {
                    if (nofArgs<i+1) {
                        std::cerr << "Error: No parameter after '--min'.\n";
                        return 1;
                    }
                    std::istringstream is(args[++i]);
                    is >> minQuality;
                    if (is.fail()) {
                        std::cerr << "Error: Illegal floating point number after '--min'.\n";
                        return 1;
                    }
                } else if (param=="--max") {
                    if (nofArgs<i+1) {
                        std::cerr << "Error: No parameter after '--max'.\n";
                        return 1;
                    }
                    std::istringstream is(args[++i]);
                    is >> maxQuality;
                    if (is.fail()) {
                        std::cerr << "Error: Illegal floating point number after '--max'.\n";
                        return 1;
                    }
                } else if (param=="--strategyStoringValueIteration") {
                    computePolicyEagerly = true;
                }

                else {
                    std::cerr << "Error: Did not understand parameter " << args[i] << std::endl;
                    return 1;
                }
            } else {
                if (baseFilename=="") {
                    baseFilename = args[i];
                } else {
                    std::cerr << "Error: More than one file name prefix given.\n";
                    return 1;
                }
            }
        }

        // Check basefilename
        if (baseFilename=="") {
            std::cerr << "Error: No input file name given.\n";
            return 1;
        }

        // Search strategy processing - including default setting
        if (searchStrategy=="") searchStrategy = "b:0.01:0.05";
        std::vector<std::tuple<char,double,double> > searchStrategyParts;
        {
            std::stringstream ssA(searchStrategy);
            std::string thisPart;
            while (std::getline(ssA,thisPart,',')) {
                std::stringstream ssB(thisPart);
                std::string partA = "";
                std::getline(ssB,partA,':');
                if (partA=="b") {
                    // Binary search
                    double a;
                    ssB >> a;
                    char sep;
                    ssB >> sep;
                    if ((ssB.fail()) or (sep!=':')) {
                        std::cerr << "Error: Illegal binary search string (1).\n";
                        return 1;
                    }
                    double b;
                    ssB >> b;
                    if (ssB.bad()) {
                        std::cerr << "Error: Illegal binary search string (2).\n";
                        return 1;
                    }
                    searchStrategyParts.push_back(std::tuple<int,double,double>('b',a,b));
                } else if (partA=="i") {
                    // Incremental
                    double a;
                    ssB >> a;
                    char sep;
                    ssB >> sep;
                    if ((ssB.fail()) or (sep!=':')) {
                        std::cerr << "Error: Illegal incremental search string (1).\n";
                        return 1;
                    }
                    double b;
                    ssB >> b;
                    if (ssB.bad()) {
                        std::cerr << "Error: Illegal incremental search string (2).\n";
                        return 1;
                    }
                    searchStrategyParts.push_back(std::tuple<int,double,double>('i',a,b));
                } else {
                    std::cerr << "Error: All strategy parts need to be 'b'inary search or 'i'ncremental: '" << partA << "'\n";
                    return 1;
                }
            }
        }

        // Start computation
        const MDP mdp(baseFilename);
        const ParityMDP parityMDP(baseFilename+".parity",mdp);
        //parityMDP.dumpDot(std::cout);
        std::pair<std::unordered_map<StrategyTransitionPredecessor,StrategyTransitionChoice,StrategyTransitionPredecessorHash>,double> bestStrategy;
        bestStrategy.second = 0.0;

        for (const std::tuple<char,double,double> &currentSearchStrategyTuple : searchStrategyParts) {

            double epsilon = std::get<2>(currentSearchStrategyTuple);

            switch (std::get<0>(currentSearchStrategyTuple)) {
            case 'i':
            {
                double mid = minQuality + std::get<1>(currentSearchStrategyTuple);
                while (mid <= 1.0) {
                    auto thisStrategy = parityMDP.computeRAPolicy(mid,epsilon,computePolicyEagerly);
                    std::cerr << "Quality computed: " << thisStrategy.second << std::endl;
                    if (thisStrategy.second>=mid) {
                        minQuality = thisStrategy.second;
                        mid = thisStrategy.second + std::get<1>(currentSearchStrategyTuple);
                        bestStrategy = thisStrategy;
                    } else {
                        // Abort. Use "min" as signalizer
                        mid = 2.0;
                    }
                }
            }
                break;
            case 'b':
            {
                while ((maxQuality-minQuality) > std::get<1>(currentSearchStrategyTuple)) {
                    double mid = (maxQuality+minQuality)/2;
                    auto thisStrategy = parityMDP.computeRAPolicy(mid,epsilon,computePolicyEagerly);
                    std::cerr << "Quality computed: " << thisStrategy.second << ", asking for " << mid << std::endl;
                    if (thisStrategy.second>=mid) {
                        // foundStrategy
                        minQuality = thisStrategy.second;
                        bestStrategy = thisStrategy;
                    } else {
                        maxQuality = mid;
                    }
                }
            }
                break;
            default:
                std::cerr << "Internal error in main.cpp,l." << __LINE__ << "\n";
                return 1;
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
