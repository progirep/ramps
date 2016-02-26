#!/usr/bin/python
#
# Simulates an MDP-Strategy

import math
import os
import sys, code
import resource
import subprocess
import signal
import tempfile
import copy
import itertools
import random
from PIL import Image
import os, pygame, pygame.locals

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

# ==================================
# Read input image
# ==================================
import os,sys
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
nofDirections = int(allParams["nofDirections"])
initX = int(allParams["initX"])
initY = int(allParams["initY"])
initDir = int(allParams["initDir"])
positionUpdateNoise = float(allParams["positionUpdateNoise"])
unicycleSpeed = float(allParams["unicycleSpeed"])

# ==================================
# Construct MDP --> States
# ==================================
with open(pngFileBasis+".sta","w") as stateFile:
    stateFile.write("(xpos,ypos,direction,color2,color3,color4,color5,color6,color7,color8)\n")
    stateMapper = {}
    for x in xrange(0,xsize):
        for y in xrange(0,ysize):
            for d in xrange(0,nofDirections):
                color = imageData[y*xsize+x]
                stateNum = len(stateMapper)
                stateFile.write(str(stateNum)+":("+str(x)+","+str(y)+","+str(d))
                for c in xrange(2,9):
                    if color==c:
                        stateFile.write(",1")
                    else:
                        stateFile.write(",0")
                stateFile.write(")\n")
                stateMapper[(x,y,d)] = stateNum

    # Add error state
    errorState = len(stateMapper)
    stateMapper[(-1,-1,-1)] = errorState
    stateFile.write(str(errorState)+":(-1,-1,-1,0,0,0,0,0,0,0)\n")


# ==================================
# Construct MDP --> Label file
# ==================================
with open(pngFileBasis+".lab","w") as labelFile:
    labelFile.write("0=\"init\" 1=\"deadlock\"\n")
    labelFile.write(str(stateMapper[(initX,initY,initDir)])+": 0\n")



# ==================================
# Construct MDP --> Transition file
# ==================================

# First, a function that computes the possible/likely
# transitions when going from a (x,y)-cell into some
# direction. It computes the image of the complete cell
# and then performs probability-weighting according to
# the areas of overlap
def computeSuccs(xpos,ypos,direction):
    minX = float(xsize)
    maxX = float(0)
    minY = float(ysize)
    maxY = float(0)
    for (x,y) in [(xpos,ypos),(xpos+1,ypos),(xpos,ypos+1),(xpos+1,ypos+1)]:
        destX = math.sin(direction/float(nofDirections)*2*math.pi)*unicycleSpeed+x
        destY = math.cos(direction/float(nofDirections)*2*math.pi)*unicycleSpeed+y
        minX = min(minX,destX)
        maxX = max(maxX,destX)
        minY = min(minY,destY)
        maxY = max(maxY,destY)
    minX -= positionUpdateNoise
    maxX += positionUpdateNoise
    minY -= positionUpdateNoise
    maxY += positionUpdateNoise
    sizeOfImage = (maxX-minX)*(maxY-minY)
    targetCells = {(-1,-1):0.0}
    for x in xrange(int(math.floor(minX)),int(math.ceil(maxX))):
        for y in xrange(int(math.floor(minY)),int(math.ceil(maxY))):
            # Compute volume of overlap
            xStart = x
            if x<minX:
                    xStart = minX
            xEnd = x+1
            if xEnd>maxX:
                    xEnd = maxX
            yStart = y
            if y<minY:
                    yStart = minY
            yEnd = y+1
            if yEnd>maxY:
                    yEnd = maxY
            thisVolume = (xEnd-xStart)*(yEnd-yStart)
            if (x>=0) and (y>=0) and (x<xsize) and (y<ysize):
                    targetCells[(x,y)] = thisVolume/sizeOfImage
            else:
                    targetCells[(-1,-1)] += thisVolume/sizeOfImage
    # print "TransitionProb from",xpos,ypos,direction," via ",minX,maxX,minY,maxY,"to:"
    # for (a,b) in targetCells.iteritems():
    #       print a,":",b
    return targetCells
                        
# Iterate over all cells and compute transition probabilities
transitionLines = []
for x in xrange(0,xsize):
    for y in xrange(0,ysize):
        for d in xrange(0,nofDirections):
                
            # Choice 0: Rotate -1
            rotMinus1 = d-1
            if rotMinus1 < 0:
                    rotMinus1 += nofDirections
            edges = computeSuccs(x,y,rotMinus1)
            for ((a,b),c) in edges.iteritems():
                    dPrime = rotMinus1
                    if (a==-1):
                            dPrime = -1
                    transitionLines.append([stateMapper[(x,y,d)],0,stateMapper[(a,b,dPrime)],c])
            
            # Choice 1: No change
            edges = computeSuccs(x,y,d)
            for ((a,b),c) in edges.iteritems():
                    dPrime = d
                    if (a==-1):
                            dPrime = -1
                    transitionLines.append([stateMapper[(x,y,d)],1,stateMapper[(a,b,dPrime)],c])

            # Choice 0: Rotate 1
            rotPlus1 = d+1
            if rotPlus1 >= nofDirections:
                    rotPlus1 -= nofDirections
            edges = computeSuccs(x,y,rotPlus1)
            for ((a,b),c) in edges.iteritems():
                    dPrime = rotPlus1
                    if (a==-1):
                            dPrime = -1
                    transitionLines.append([stateMapper[(x,y,d)],2,stateMapper[(a,b,dPrime)],c])
                                
# Print transitions file: It contains the transitions computed earlier PLUS an error state self loop
with open(pngFileBasis+".tra","w") as transitionFile:
    transitionFile.write(str(len(stateMapper))+" "+str(len(stateMapper)*3-2)+" "+str(len(transitionLines)+1)+"\n")
    for (a,b,c,d) in transitionLines:
        transitionFile.write(str(a)+" "+str(b)+" "+str(c)+" "+str(d)+"\n")
    transitionFile.write(str(errorState)+" 0 "+str(errorState)+" 1.0\n")


# ==================================
# Compute and read strategy/policy
# ==================================
rampsProcess = subprocess.Popen(["../../src/ramps",pngFileBasis], bufsize=1048768, stdin=None, stdout=subprocess.PIPE)
policy = {}
currentPolicyState = None
nofPolicyStates = int(rampsProcess.stdout.readline().strip())
while True:
    line = rampsProcess.stdout.readline()
    if line != '':
        if line.startswith("->"):
           line = line[2:].strip().split(" ")
           assert len(line)==3
           policy[currentPolicyState][2][int(line[0])] = (int(line[1]),int(line[2]))
        else:
            print >>sys.stderr, "L:",line
            line = line.strip().split(" ")
            assert len(line)==4
            currentPolicyState = (int(line[0]),int(line[1]))
            policy[currentPolicyState] = [int(line[2]),int(line[3]),{}]
    else:
        break
returncode = rampsProcess.wait()
if (returncode!=0):
    print >>sys.stderr, "RAMPS returned error code:",returncode
    sys.exit(1)

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

        # Update 
        if resetInThisRound or (policyState==None):
                policyState = 0
                policyData = 0

        # Obtain robot information for drawing
        (robotX,robotY,direction) = reverseStateMapper[policy[(policyState,policyData)][0]]
            
        # Draw Field
        for x in xrange(0,xsize):
            for y in xrange(0,ysize):
                paletteColor = imageData[y*xsize+x]
                color = palette[paletteColor*3:paletteColor*3+3]
                pygame.draw.rect(screenBuffer,color,((x+1)*MAGNIFY,(y+1)*MAGNIFY,MAGNIFY,MAGNIFY),0)
                
        # Draw boundary
        if robotX==-1:
            boundaryColor = (255,0,0)
        else:
            boundaryColor = (64,64,64)
            pygame.draw.rect(screenBuffer,boundaryColor,(0,0,MAGNIFY*(xsize+2),MAGNIFY),0)
            # pygame.draw.rect(screenBuffer,boundaryColor,(0,0,MAGNIFY*(xsize+2),MAGNIFY),0)


        # Draw "Good" Robot
        pygame.draw.circle(screenBuffer, (192,32,32), ((robotX+1)*MAGNIFY+MAGNIFY/2,(robotY+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-2, 0)
        pygame.draw.circle(screenBuffer, (255,255,255), ((robotX+1)*MAGNIFY+MAGNIFY/2,(robotY+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3-1, 1)
        pygame.draw.circle(screenBuffer, (0,0,0), ((robotX+1)*MAGNIFY+MAGNIFY/2,(robotY+1)*MAGNIFY+MAGNIFY/2) , MAGNIFY/3, 1)

        # Draw cell frames
        for x in xrange(0,xsize):
            for y in xrange(0,ysize):
                pygame.draw.rect(screenBuffer,(0,0,0),((x+1)*MAGNIFY,(y+1)*MAGNIFY,MAGNIFY,MAGNIFY),1)
        pygame.draw.rect(screenBuffer,(0,0,0),(MAGNIFY-1,MAGNIFY-1,MAGNIFY*xsize+2,MAGNIFY*ysize+2),1)
        
        # Flip!
        screen.blit(screenBuffer, (0, 0))
        pygame.display.flip()

        # Update state
        randomNumber = random.random()
        (mdpstate,decision,dataUpdate) = policy[(policyState,policyData)]
        transitionList = transitionLists[(mdpstate,decision)]
        dest = None
        for (a,b) in transitionList:
            print "TL",(a,b)
            if randomNumber<=b:
                dest = a
                randomNumber = 123.0
            else:
                randomNumber -= b
        # Rounding error?
        if (dest==None):
            dest = transitionList[0][0]
        # Update memory
        print policyState
        print decision
        print dest
        print policy[(policyState,policyData)]
        assert dest in policy[(policyState,policyData)][2]
        (policyState,policyData) = dataUpdate[dest]
                            
        # Make the transition
        if not isPaused:
            # Done
            clock.tick(10)
        else:
            clock.tick(3)


# ==================================
# Call main program
# ==================================
actionLoop()
