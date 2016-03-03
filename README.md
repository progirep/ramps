RAMPS - RA MDP Solver
==============================

(C) 2016 by Ruediger Ehlers, licensed under GPLv3


Installation
============

Requirements
------------
- A moderately modern C++ and C compiler installed in a Unix-like environment. Linux and MacOS should be fine.
- For the examples, an installation of Python 2, version 2.7 or above, is also needed, including an installation of the PIL and pygame libraries.

Building RAMPS on Linux
-----------------------
In order to build RAMPS, open a terminal in the RAMPS directory and type:

> cd src
> g++ -O3 -fopenmp -std=c++11 -march=native *.cpp -o ramps

Internally, RAMPS uses value iteration to solve MDPs. The value iteration is parallelized on multi-core computers. If this is not desired, the "-openmp" parameter can be omitted in the command above in order to obtain a single-threaded binary. Note that when multi-threading, value iteration may occasionally abort earlier, which makes the strat

Usage 
============


Input file format
-----------------
RAMPS takes MDPs in the file format that the probabilistic model checker PRISM exports. MDP consists of three input files:

- A ".lab" label file
- A ".sta" state list file
- A ".tra" transition file

A state file consists of a list of states and their meanings. It starts with a line of the form "(compA,compB,...)" that explains the components of the state description. From the second line onwards, the file contains a list of states and the valuations of the state components. All states must be given in ascending number, starting from 0. For example, a state file may look as follows:

> (request,grant)
> 0:(false,false)
> 1:(true,false)
> 2:(false,true)
> 3:(true,true)

A label file marks certain states are being special. RAMPS only uses this file to obtain the information which state is initial. The first line of the label file encodes which number identifier describes which state property. Only files that start with 

> 0="init" [other Stuff]

are supported. After the first line, the label file contains lines that mark states. The lines start with the state number, followed by a ': ', followed by a space-separated list of markings. For example, a label file may look as follows:

> 0="init" 1="deadlock"
> 0: 1
> 2: 0

In this example, the initial state is the one with number 0.

The transition file is normally the largest file. It contains all MDP transitions. The first line in a transition file contains three space-separated integer numbers:

- The number of states
- The number of transitions
- The number of edges in all transitions
 
All following lines are of the form "<source state><transition number><target state><transition probabality>[action]". The action name is optional. All transitions are numbered consecutively per source state, always starting with number 0. For every combination of source state and transition number, the sum of probabilities of the transitions must be 1.0 (or very close to it). Also, the edges of a source state/transition number combination must be listed consecutively in the input file. If a transition is labeled by an action, then all edges of the same transition must be labeled by the action. The "examples" directory or of RAMPS contains some example transition list files.

In addition to an MDP, RAMPS needs an input file with a deterministic parity automaton (extension ".parity"). The first line of a parity automaton file contains a space-separated list of parity state colors. RAMPS uses the parity automaton semantics that the automaton accepts all traces for which the highest color occurring infinitely often is even. After the first line, the parity automaton file contains the automaton transitions. Every such line is of the form "<source state><label><destination state>". The label is either an action mentioned in the MDP transition file or an expression of the form "component=value", where component is a state components mentioned in the state file, and value is a corresponding value. Note that values are not interpreted and treated as strings, so "count=2" and "count=2.0" are different labels. A parity automaton takes a transition whenever it can: an action-labeled transition is taken if in the MDP, a transition with the action is taken, and a state-component-value-labeled transition is taken whenever the MDP transitions to a state that satisfies the constraint. The parity automaton has implicit self-loops for the case that no other transition is applicable.


Computing policies
------------------

When the state file, label file, transition file, and parity automaton file are ready, RAMPS can be called. If these files are called "/path/to/inputFile.sta", "/path/to/inputFile.lab", "/path/to/inputFile.tra", and "/path/to/inputFile.parity", RAMPS is called as follows:

> /path/to/ramps /path/to/inputFile [parameters] > /path/to/outputFile.strategy

The tool outputs a couple of status lines and the RA-level of the generated strategy in the error stream. The strategy itself is written to the standard output stream, which is why it need to be redirected with ">". 

Internally, RAMPS searches for values of "p" such that a "p"-RA policy exist. For checking the existance of a "p"-RA policy, it furthermore employs value iteration, which terminates as soon as the sum of probability updates in a step is below a threshold. Both the search strategy and the threshold can be specified with the "--ses" parameter. There are two search strategies:

- In a binary search strategy, RAMPS tries to quickly approximate the highest error resilience level. The parameter "--ses" is followed by a tuple of the form "b:<cutoff>:<threshold>" in this case, where "cutoff" specifies the difference between the minimal and maximal implementable error-resilience levels at which the search terminates.
- In an incremental search strategy, RAMPS tries to successively find strategies that are a bit better than the policies found before. As the policies found by RAMPS can have a higher RA level than requested by the search strategy, incremental search can sometimes find good policies relatively quickly. The parameter "--ses" is followed by a tuple of the form "i:<increment>:<threshold>", where the increment denotes the required improvement in the RA level before the search terminates.

Strategies can be also be sequentally combined, and the search strategy configuration are separated by commas. For example, the call

> ./ramps example --ses b:0.05:1,i:0.0001:0.01 

specifies a search strategy in which first binary search is employed until the difference between the upper and lower known bounds on the attainable RA level is smaller than 0.05 while using a value iteration threshold of 1. Afterwards, RAMPS will try to successively improve the RA level of the best known policy so far in steps of 0.0001 each, while using the lower value iteration termination threshold of 0.01.

The optimal RA search process can also be influenced with the "--min" and "--max" parameters, which can used to report minimal and maximal implementable RA levels to RAMPS in order to speed up the search process.

For example, the following command can be used if it is known that the best attainable RA level should be somewhere between 0.3 and 0.95:

> ./ramps example --ses b:0.05:1,i:0.0001:0.01 --min 0.3 --max 0.95


Output Policies
---------------
An output strategy (file) starts with a number on a single line. It represents the number of states in the policy. Each state may be additionally parametrized by some data value.

After the first line, the behavior of the policy is given in the strategy file. It consists of multiple blocks that describe the actions of the policy in each of its state. Every block starts with a line with four number that represent

1. the number of the state for which the block describes the behavior,
2. the data value under which the block is executed,
3. the state in which the MDP needs to be if the strategy in the state, and
4. the transition out of the MDP state chosen by the policy.

After the initial line of the block, a couple of lines starting with a "->" each follow. These describe for every successor MDP state to which state the strategy transitions and how the data value of the policy is updated. 


Demos
=====

Unicycle
--------
To demonstrate the behavior of computed MDPs, the RAMPS distributed comes with a unicycle-dynamics robot control simulation script. It reads a grid-based scenario for this setting, calls RAMPS to compute a control policy, and then displays a "pygame"-based simulator to demonstrate the generated policy. Each scenario consists of three files:

- A 256-color PNG file that encodes the properties of the grid. Color 0 denotes free space, color 1 is used for static obstacle, and color 2-7 are
  reserved to encode regions that can be referred to in the specification.
- A ".params" file that contains a couple of simulation/MDP computation parameters.
- A ".parity" file with the specification. The atoms "color2=0","color2=1",...,"color7=0","color7=1" can be used as atoms to control the transitions
  in the parity automaton.
  
The files "scenario.params", "scenario.png", and "scenario.parity" in the "examples/simulated-unicycle" directory contain an example. The "scenario.param" contains a description of all parameters.

A scenario is executed by running

> ./simulator.py <inputFile.png> [paramsToRAMPS]

The names of the files other than the PNG image are inferred from the image file name. The parameters to RAMPS are forwarded without modification. Running the simulator creates the transition, state, and label files for the MDP in the same directory. The strategy is written to a strategy file, so that when the simulator is run for the next time, the cache strategy is used, *regardless of whether different parameters to RAMPS have been given.* When changing the parameters, the strategy file should be deleted manually.

When the simulator window is displayed after strategy computation, the following keys control the simulator:

- Q or ESC: these keys terminate the simulation session
- +: This key speeds up the simulation.
- -: This key slows down the simulation.
- SPACE: This key pauses and unpauses the simulation
- R: This key resets the simulation. This is especially helpful after a crash of the robot (which is represented by drawing the workspace on a red background)

Note that if the call to RAMPS fails during the execution of "simulator.py", the generated strategy file must be deleted before "simulator.py" can be (sucessfully) called again.
