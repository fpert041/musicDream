#pragma once

#include "ofMain.h"
#include "ofxOscParameterSync.h"
#include "ofxGui.h"
#include "maximilian.h"
#include "ofxMaxim.h"
#include "GrainPlayer.h"
#include "ofSoundUnit.h"
#include "ofxMaxim.h"
#include "ofxMaxim.h"
#include "maxiGrains.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        void audioIn(float * input, int bufferSize, int nChannels);
        void audioOut(float * output, int bufferSize, int nChannels);
    
    void updateGrainPlayerVariables();
      //  void play(double *output);
		
		ofxOscParameterSync sync;
		ofParameterGroup parameters;

        //Grain Variables
        ofParameter<float> speed;
        ofParameter<float> grainSize;
        ofParameter<float> pitch;
        ofParameter<float> playHead;
        ofParameter<float> overlaps;
        ofParameter<float> volume;

        //Recording
        ofParameter<bool> bSetPosition;
        ofParameter<bool> bRecLiveInput;
    
        //NEW pameters:
        ofParameter<bool> bMouseControl;
    
        ofxPanel gui;
    
    
    
   // maxiSample beats; //We give our sample a name. It's called beats this time. We could have loads of them, but they have to have
    
    ofxMaxiFilter myFilter, myFilter2;
    
    
    int sampleRate;
    int bufferSize;
    ofSoundStream soundStream;
    ofSoundMixer mixer;
    
    vector <float> lAudio;
    vector <float> rAudio;
    
    GrainPlayer grainPlayer;
    
    //GUI
 //   ofxUICanvas *gui;
//  void guiEvent(ofxUIEventArgs &e);

};
