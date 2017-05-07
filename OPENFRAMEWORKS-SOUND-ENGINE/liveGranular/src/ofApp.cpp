#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    
    sampleRate 	= 44100;
    bufferSize = 512;
    
    lAudio.assign(bufferSize, 0.0);
    rAudio.assign(bufferSize, 0.0);

	soundStream.listDevices();

	ofSetFrameRate(60);
    
    grainPlayer.setup();
    mixer.addInputFrom(&grainPlayer);
    
    //SoundStream
    soundStream.printDeviceList();
    soundStream.setup(this, 2, 2, sampleRate, bufferSize, 4);
    //soundStream.setOutput(this); // un-commented
    soundStream.start();
    
	parameters.setName("parameters");
    parameters.add(speed.set("speed",0.8,-4.0,4.0));
    parameters.add(pitch.set("pitch",1.15,0.0,10.0));
    parameters.add(grainSize.set("grainSize",0.325,0.025,0.45));
    parameters.add(overlaps.set("overlaps",5.4,1.0,1000.0));
    
    parameters.add(bSetPosition.set("bSetPosition",false));
    parameters.add(bRecLiveInput.set("bRecLiveInput",true));
    parameters.add(playHead.set("playHead",0.0,0.0,1.0));
    parameters.add(volume.set("volume",0.5,0.0,1.0));
    
    parameters.add(bMouseControl.set("bMouseControl",false));
    
    gui.setup(parameters);
	// by now needs to pass the gui parameter groups since the panel internally creates it's own group
	string host = "localhost";
	//host = "192.168.1.5"; //external device test
	int inPort = IN_PORT;
	sync.setup((ofParameterGroup&)gui.getParameter(), inPort, host, OUT_PORT);
	ofSetVerticalSync(true);
    
//    beats.load("Eweline_010.wav");//load in your samples. Provide the full path to a wav file.
//    printf("Summary:\n%s", beats.getSummary());//get info on samples if you like.
}

//--------------------------------------------------------------
void ofApp::update(){
	sync.update();
	
    updateGrainPlayerVariables();
    grainPlayer.updatePlayHead();
}

void ofApp::updateGrainPlayerVariables(){
    
    grainPlayer.speed = speed;
    grainPlayer.pitch = pitch;
    grainPlayer.grainSize = grainSize;
    grainPlayer.overlaps = overlaps;
    grainPlayer.bRecLiveInput = bRecLiveInput;
    grainPlayer.bSetPosition = bSetPosition;
    grainPlayer.playHead = playHead;
    grainPlayer.volume = volume;
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(100);
    
	gui.draw();
    grainPlayer.draw();

	string inPort = to_string(IN_PORT);
	string outPort = to_string(OUT_PORT);

	ofDrawBitmapString("listening to port: " + inPort, ofGetWidth() - 240., 50.);
	ofDrawBitmapString("sending to port: " + outPort, ofGetWidth() - 240., 80.);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if( key == 's' ){
        soundStream.start();
    }
    
    if( key == 'e' ){
        soundStream.stop();
    }
    
    if( key == 'm' ){
        bMouseControl = !bMouseControl;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
    float x1 = myFilter.lopass(x, 0.25);
    float y1 = myFilter2.lopass(y, 0.25);

    if (bMouseControl)
    {
        speed = ((double ) x1 / ofGetWidth() * 4.0) - 2.0;
        grainSize = ((double) y1 / ofGetHeight() * 0.1) + 0.001;
        
        if (grainSize < 0.025 ) grainSize = 0.025;
        if (grainSize > 0.45 ) grainSize = 0.45;
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void ofApp::audioIn(float * input, int bufferSize, int nChannels)
{
    grainPlayer.audioReceived(input,bufferSize,nChannels);


}

//--------------------------------------------------------------
double sample = 0.;
void ofApp::audioOut(float * output, int bufferSize, int nChannels)
{

	for (int i = 0; i < bufferSize; i++) { // This seems to activate/initialise the audio OUT on windows

		lAudio[i] = output[i*nChannels] = sample;
		rAudio[i] = output[i*nChannels + 1] = sample;
	}

	// write mixer output (containing the granulated sound) into out buffer
	mixer.audioRequested(output, bufferSize, nChannels);
}

//void ofApp::play(double *output) {//this is where the magic happens. Very slow magic.
//    
//    //output[0]=beats.play();//just play the file. Looping is default for all play functions.
//    output[0]=beats.play(0.68);//play the file with a speed setting. 1. is normal speed.
//    //output[0]=beats.play(0.5,0,44100);//linear interpolationplay with a frequency input, start point and end point. Useful for syncing.
//    //output[0]=beats.play4(0.5,0,44100);//cubic interpolation play with a frequency input, start point and end point. Useful for syncing.
//    
//    output[1]=output[0];
//}
