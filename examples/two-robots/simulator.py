#!/usr/bin/python
#
# Simulates an MDP-Strategy

import math
import os
import sys
import resource
import subprocess
import signal
import tempfile
import copy
import itertools
import random
from PIL import Image
import pygame, pygame.locals

# ==================================
# Settings
# ==================================
MAGNIFY = 64

# ==================================
# Entry point
# ==================================
if len(sys.argv)<2:
    print >>sys.stderr, "Error: Need PNG file as parameter"
    sys.exit(1)
specFile = sys.argv[1]
rampsParameters = sys.argv[2:]

# ==================================
# Read input image
# ==================================
pngfile = Image.open(specFile)
pngFileBasis = specFile[0:specFile.rfind(".png")]
# print "Size of Workspace:",pngfile.size
xsize = pngfile.size[0]
ysize = pngfile.size[1]
imageData = pngfile.getdata()
palette = pngfile.getpalette()
if (xsize>1023):
    print >>sys.stderr,"Error: Scenario is too large - not supported."
    sys.exit(1)
if (ysize>1023):
    print >>sys.stderr,"Error: Scenario is too large - not supported."
    sys.exit(1)


# ==================================
# Read parameter file
# ==================================
parameterFileName = pngFileBasis+".params"
allParams = {}
for a in open(parameterFileName,"r").readlines():
    a = a.strip()
    if len(a)>0 and a[0]!='#':
        posEqual = a.index("=")
        allParams[a[0:posEqual].strip()] = a[posEqual+1:].strip()

# ==================================
# Parse parameter file
# ==================================
initXA = int(allParams["initXA"])
initYA = int(allParams["initYA"])
initXB = int(allParams["initXB"])
initYB = int(allParams["initYB"])
positionUpdateNoise = float(allParams["positionUpdateNoise"])

# ==================================
# Construct MDP --> States
# ==================================
with open(pngFileBasis+".sta","w") as stateFile:
    stateFile.write("(xposA,yposA,xposB,yposB,color2A,color3A,color4A,color5A,color6A,color7A,color8A,color2B,color3B,color4B,color5B,color6B,color7B,color8B,carry,carrySuccess)\n")
    stateMapper = {}
    for xA in xrange(0,xsize):
        for yA in xrange(0,ysize):
            for xB in xrange(0,xsize):
                for yB in xrange(0,ysize):
                    if xA!=xB or yA!=yB:
                        if (imageData[xA+yA*xsize]!=1) and (imageData[xB+yB*xsize]!=1):
                            carryModes = [(0,0)]
                            if xB==xA+2 and yA==yB:
                                carryModes.append((1,0))
                                if (imageData[xA+1+yA*xsize]==3):
                                    carryModes.append((0,1))
                            for (a,b) in carryModes:
                                colorA = imageData[yA*xsize+xA]
                                stateNum = len(stateMapper)
                                stateFile.write(str(stateNum)+":("+str(xA)+","+str(yA)+","+str(xB)+","+str(yB))
                                for c in xrange(2,9):
                                    if colorA==c:
                                        stateFile.write(",1")
                                    else:
                                        stateFile.write(",0")
                                colorB = imageData[yB*xsize+xB]
                                for c in xrange(2,9):
                                    if colorB==c:
                                        stateFile.write(",1")
                                    else:
                                        stateFile.write(",0")
                                stateFile.write(","+str(a)+","+str(b)+")\n")
                                stateMapper[(xA,yA,xB,yB,a,b)] = stateNum

    # Add error state
    errorState = len(stateMapper)
    errorStateKey = (-1,-1,-1,-1,-1,-1)
    stateMapper[errorStateKey] = errorState
    stateFile.write(str(errorState)+":(-1,-1,-1,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)\n")


# ==================================
# Construct MDP --> Label file
# ==================================
with open(pngFileBasis+".lab","w") as labelFile:
    labelFile.write("0=\"init\" 1=\"deadlock\"\n")
    labelFile.write(str(stateMapper[(initXA,initYA,initXB,initYB,0,0)])+": 0\n")


# ==================================
# Construct MDP --> Transition file
# ==================================

# First, a function that computes the possible/likely
# transitions when going from a (x,y)-cell into some
# direction. It computes the image of the complete cell
# and then performs probability-weighting according to
# the areas of overlap
def computeSuccs(xpos,ypos,direction):

    # If direction is "4", this means no move
    if (direction==4):
        return [(xpos,ypos,1.0)]

    succs = []
    errorProb = 0.0
    probs = [positionUpdateNoise/3.0,positionUpdateNoise/3.0,positionUpdateNoise/3.0,positionUpdateNoise/3.0]
    probs[direction] = 1.0-positionUpdateNoise
    if (xpos>0) and imageData[xpos-1+ypos*xsize]!=1:
        succs.append((xpos-1,ypos,probs[0]))
    else:
        errorProb += probs[0]
    if (xpos<xsize-1) and imageData[xpos+1+ypos*xsize]!=1:
        succs.append((xpos+1,ypos,probs[1]))
    else:
        errorProb += probs[1]
    if (ypos>0) and imageData[xpos+(ypos-1)*xsize]!=1:
        succs.append((xpos,ypos-1,probs[2]))
    else:
        errorProb += probs[2]
    if (ypos<ysize-1) and imageData[xpos+(ypos+1)*xsize]!=1:
        succs.append((xpos,ypos+1,probs[3]))
    else:
        errorProb += probs[3]
    if errorProb > 0.0:
        succs.append((-1,-1,errorProb))
    return succs


# Iterate over all cells and compute transition probabilities
transitionLines = []
overallNofTransitions = 0
for xA in xrange(0,xsize):
    for yA in xrange(0,ysize):
        for xB in xrange(0,xsize):
            for yB in xrange(0,ysize):
                if xA!=xB or yA!=yB:
                    if (imageData[xA+yA*xsize]!=1) and (imageData[xB+yB*xsize]!=1):

                        # Which current carry modes are possible for this combination?
                        carryModes = [0]
                        if xB==xA+2 and yA==yB:
                            carryModes.append(1)

                        # Normal motion.
                        for carryMode in carryModes:
                            sourceState = stateMapper[(xA,yA,xB,yB,carryMode,0)]
                            overallNofTransitions += 25
                            for dirA in [0,1,2,3,4]: # Action 4 is standing still
                                for dirB in [0,1,2,3,4]: # Action 4 is standing still
                                    succA = computeSuccs(xA,yA,dirA)
                                    succB = computeSuccs(xB,yB,dirB)
                                    errorProb = 0.0
                                    carryingSelfTransitionProb = 0.0
                                    thisAction = dirA*5+dirB
                                    for (destXA,destYA,probA) in succA:
                                        for (destXB,destYB,probB) in succB:
                                            if destXB!=destXA+2 or destYA!=destYB:
                                                destCarryMode = 0
                                            else:
                                                destCarryMode = carryMode
                                            if destXA==-1 or destXB==-1:
                                                errorProb += probA*probB
                                            elif destXA==destXB and destYA==destYB:
                                                if carryMode==1:
                                                    carryingSelfTransitionProb += probA*probB
                                                else:
                                                    errorProb += probA*probB
                                            elif (imageData[destXA+destYA*xsize]==1) or (imageData[destXB+destYB*xsize]==1):
                                                errorProb += probA*probB
                                            else:
                                                transitionLines.append([sourceState,thisAction,stateMapper[(destXA,destYA,destXB,destYB,destCarryMode,0)],probA*probB])
                                    if errorProb>0:
                                        transitionLines.append([sourceState,thisAction,errorState,errorProb])
                                    if carryingSelfTransitionProb>0:
                                        transitionLines.append([sourceState,thisAction,sourceState,carryingSelfTransitionProb])

                        # Picking up
                        if xB==xA+2 and yA==yB and (imageData[xA+1+yA*xsize]==2):
                            sourceState = stateMapper[(xA,yA,xB,yB,0,0)]
                            destState = stateMapper[(xA,yA,xB,yB,1,0)]
                            transitionLines.append([sourceState,25,destState,1.0])
                            overallNofTransitions += 1

                        # Dropping at the destination
                        if xB==xA+2 and yA==yB and (imageData[xA+1+yA*xsize]==3):
                            sourceState = stateMapper[(xA,yA,xB,yB,1,0)]
                            destState = stateMapper[(xA,yA,xB,yB,0,1)]
                            transitionLines.append([sourceState,25,destState,1.0])

                            # Recover after drop
                            sourceState = stateMapper[(xA,yA,xB,yB,0,1)]
                            destState = stateMapper[(xA,yA,xB,yB,0,0)]
                            transitionLines.append([sourceState,0,destState,1.0])
                            overallNofTransitions += 2


# Print transitions file: It contains the transitions computed earlier PLUS an error state self loop
with open(pngFileBasis+".tra","w") as transitionFile:
    transitionFile.write(str(len(stateMapper))+" "+str(overallNofTransitions+1)+" "+str(len(transitionLines)+1)+"\n")
    for (a,b,c,d) in transitionLines:
        transitionFile.write(str(a)+" "+str(b)+" "+str(c)+" "+str(d)+"\n")
    transitionFile.write(str(errorState)+" 0 "+str(errorState)+" 1.0\n")


# ==================================
# Compute and read strategy/policy
# ==================================
if not os.path.exists(pngFileBasis+".strategy") or (os.path.getmtime(pngFileBasis+".params")>os.path.getmtime(pngFileBasis+".strategy")):
    with open(pngFileBasis+".strategy","wb") as out:
        rampsProcess = subprocess.Popen(["../../src/ramps",pngFileBasis]+rampsParameters, bufsize=1048768, stdin=None, stdout=out)
        returncode = rampsProcess.wait()
        if (returncode!=0):
            print >>sys.stderr, "RAMPS returned error code:",returncode
            sys.exit(1)

policy = {}
currentPolicyState = None
with open(pngFileBasis+".strategy","r") as strat:
    nofPolicyStates = int(strat.readline().strip())
    while True:
        line = strat.readline()
        if line != '':
            if line.startswith("->"):
               line = line[2:].strip().split(" ")
               assert len(line)==3
               policy[currentPolicyState][2][int(line[0])] = (int(line[1]),int(line[2]))
            else:
                line = line.strip().split(" ")
                assert len(line)==4
                currentPolicyState = (int(line[0]),int(line[1]))
                policy[currentPolicyState] = [int(line[2]),int(line[3]),{}]
        else:
            break

# ==================================
# Prepare reverse state mapper and
# Searchable transition list
# ==================================
reverseStateMapper = {}
for (a,b) in stateMapper.iteritems():
    reverseStateMapper[b] = a
transitionLists = {}
for (a,b,c,d) in transitionLines:
    if not (a,b) in transitionLists:
        transitionLists[(a,b)] = [(c,d)]
    else:
        transitionLists[(a,b)].append((c,d))

# =========================================
# Initialize interactive display
# =========================================
pygame.init()
displayInfo = pygame.display.Info()
MAGNIFY = min(MAGNIFY,displayInfo.current_w*3/4/xsize)
MAGNIFY = min(MAGNIFY,displayInfo.current_h*3/4/ysize)


# ==================================
# Main loop
# ==================================
def actionLoop():
    screen = pygame.display.set_mode(((xsize+2)*MAGNIFY,(ysize+2)*MAGNIFY))
    pygame.display.set_caption('Policy Visualizer')
    clock = pygame.time.Clock()

    screenBuffer = pygame.Surface(screen.get_size())
    screenBuffer = screenBuffer.convert()
    screenBuffer.fill((64, 64, 64)) # Dark Gray

    # Initialize Policy
    policyState = None
    policyData = None

    isPaused = False
    speed = 10
    while 1:

        resetInThisRound = False

        # Process events
        for event in pygame.event.get():
            if event.type == pygame.locals.QUIT or (event.type == pygame.locals.KEYDOWN and event.key in [pygame.locals.K_ESCAPE,pygame.locals.K_q]):
                return
            if (event.type == pygame.locals.KEYDOWN and event.key == pygame.locals.K_SPACE):
                isPaused = not isPaused
            if (event.type == pygame.locals.KEYDOWN and event.key == pygame.locals.K_r):
                resetInThisRound = True
            if (event.type == pygame.locals.KEYDOWN and event.key == pygame.locals.K_PLUS):
                speed += 1
            if (event.type == pygame.locals.KEYDOWN and event.key == pygame.locals.K_MINUS):
                speed = max(speed-1,1)

        # Update
        if resetInThisRound or (policyState==None):
                policyState = 0
                policyData = 0

        # Obtain robot information for drawing
        if (policyState,policyData) in policy:
            (robotXA,robotYA,robotXB,robotYB,carryA,carryB) = reverseStateMapper[policy[(policyState,policyData)][0]]
        else:
            (robotXA,robotYA,robotXB,robotYB,carryA,carryB) = (-1,-1,-1,-1,-1,-1) # Crashed

        # Draw Field
        for x in xrange(0,xsize):
            for y in xrange(0,ysize):
                paletteColor = imageData[y*xsize+x]
                color = palette[paletteColor*3:paletteColor*3+3]
                pygame.draw.rect(screenBuffer,color,((x+1)*MAGNIFY,(y+1)*MAGNIFY,MAGNIFY,MAGNIFY),0)

        # Draw boundary
        if robotXA==-1:
            boundaryColor = (255,0,0)
        else:
            boundaryColor = (64,64,64)
        pygame.draw.rect(screenBuffer,boundaryColor,(0,0,MAGNIFY*(xsize+2),MAGNIFY),0)
        pygame.draw.rect(screenBuffer,boundaryColor,(0,MAGNIFY,MAGNIFY,MAGNIFY*(ysize+1)),0)
        pygame.draw.rect(screenBuffer,boundaryColor,(MAGNIFY*(xsize+1),MAGNIFY,MAGNIFY,MAGNIFY*(ysize+1)),0)
        pygame.draw.rect(screenBuffer,boundaryColor,(MAGNIFY,MAGNIFY*(ysize+1),MAGNIFY*xsize,MAGNIFY),0)
        # pygame.draw.rect(screenBuffer,boundaryColor,(0,0,MAGNIFY*(xsize+2),MAGNIFY),0)


        # Draw "Good" Robot
        if robotXA!=-1:
            pygame.draw.circle(screenBuffer, (192,32,32), ((robotXA+1)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-2, 0)
            pygame.draw.circle(screenBuffer, (255,255,255), ((robotXA+1)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-1, 1)
            pygame.draw.circle(screenBuffer, (0,0,0), ((robotXA+1)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3, 1)

            pygame.draw.circle(screenBuffer, (192,192,32), ((robotXB+1)*MAGNIFY+MAGNIFY/2,(robotYB+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-2, 0)
            pygame.draw.circle(screenBuffer, (255,255,255), ((robotXB+1)*MAGNIFY+MAGNIFY/2,(robotYB+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-1, 1)
            pygame.draw.circle(screenBuffer, (0,0,0), ((robotXB+1)*MAGNIFY+MAGNIFY/2,(robotYB+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3, 1)

        # Freight
        if carryA==1:
            pygame.draw.circle(screenBuffer, (30,192,192), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-2, 0)
            pygame.draw.circle(screenBuffer, (255,255,255), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-1, 1)
            pygame.draw.circle(screenBuffer, (0,0,0), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3, 1)
        elif carryB==1:
            pygame.draw.circle(screenBuffer, (200,200,200), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-2, 0)
            pygame.draw.circle(screenBuffer, (255,255,255), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-1, 1)
            pygame.draw.circle(screenBuffer, (0,0,0), ((robotXA+2)*MAGNIFY+MAGNIFY/2,(robotYA+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3, 1)


        # Draw cell frames
        for x in xrange(0,xsize):
            for y in xrange(0,ysize):
                pygame.draw.rect(screenBuffer,(0,0,0),((x+1)*MAGNIFY,(y+1)*MAGNIFY,MAGNIFY,MAGNIFY),1)
        pygame.draw.rect(screenBuffer,(0,0,0),(MAGNIFY-1,MAGNIFY-1,MAGNIFY*xsize+2,MAGNIFY*ysize+2),1)

        # Flip!
        screen.blit(screenBuffer, (0, 0))
        pygame.display.flip()

        # Update state
        if (not isPaused) and robotXA!=-1:
            randomNumber = random.random()
            (mdpstate,decision,dataUpdate) = policy[(policyState,policyData)]
            transitionList = transitionLists[(mdpstate,decision)]
            dest = None
            for (a,b) in transitionList:
                if randomNumber<=b:
                    dest = a
                    randomNumber = 123.0
                else:
                    randomNumber -= b
            # Rounding error?
            if (dest==None):
                dest = transitionList[0][0]
            # Update memory
            # print policyState
            # print decision
            # print dest
            # print policy[(policyState,policyData)]
            assert dest in policy[(policyState,policyData)][2]
            (policyState,policyData) = dataUpdate[dest]
            # print "MDP", mdpstate, "PS/Data:",policyState,",",policyData, "decision",decision,"dataupdate",dataUpdate

        # Make the transition
        if not isPaused:
            # Done
            clock.tick(speed)
        else:
            clock.tick(3)


# ==================================
# Call main program
# ==================================
actionLoop()
