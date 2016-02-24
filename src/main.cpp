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
        parityMDP.dumpDot(std::cout);

    } catch (const char *error) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    } catch (const std::string error) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }

}
