//
//  GrainPlayer.h
//  testCpp11
//
//  Created by Joshua Batty on 4/06/14.
//
//  original post at https://forum.openframeworks.cc/t/live-audio-granular-synthesis-with-ofxmaxim/16100
//
//
//  Reworked by Pawel Dziadur and Francesco Perticarari (2017)
//  for OpenFrameworks 9 compatibility (tested on Mac OS El Capitan).
//  requires modified addons:
//  ofxDspChainPawelFrancesco
//  and ofxMaximLoopRecordHack

#pragma once


#include "ofSoundUnit.h"
#include "ofMain.h"
#include "ofxMaxim.h"
#include "maxiGrains.h"
#include <time.h> // <-------------------------------------------------------------

typedef hannWinFunctor grainPlayerWin;

#define LENGTH 294000

class GrainPlayer : public ofSoundSource{
    
public:
    void setup();
    void draw();
    
    void drawWaveform();
    
	void audioReceived(float * input, int bufferSize, int nChannels);
    void audioRequested (float * output, int numFrames, int nChannels);
    void setSampleRate( int rate );
	string getName() { return "soundSourceMaxiGrains"; }
    
    //Bool event for Recording Toggle/ resets play position relative to current Rec position
    void updatePlayHead();
    bool bUpdatePlayheadEvent;

    int bufferSize;
	int	sampleRate;
	float volume;
    
    //////////// ofxMaxim ////////////
    //maxiPitchStretch<grainPlayerWin> *ps;
    
    maxiMix mymix;
    maxiSample samp;
    
    double wave;
    double outputs[2];
    double windowAmp;
    maxiTimePitchStretch<grainPlayerWin, maxiSample> *ps;
    
    //Recording
    bool bSetPosition;
    bool bRecLiveInput;
    float recMix;
    
    
    //Grain Variables
    float speed;
    float grainSize;
    float pitch;
    float playHead;
    float overlaps;
    
    //Drawing
    int curXpos, curYpos;
    int prevXpos, prevYpos;
};
