# Music Leap (r)
## by Francesco Perticarari (c) 2017

### An Augmented Reality prototype controller for music making
#### Powered by: MS Visual Stidio 2017, Unreal Engine 4, C++, OpenFrameworks, and OSC.

![Alt text](img/FRONTPAGE.PNG?raw=true "Optional Title")

This Project Requires the ARToolkitPlugin and the UE4OSC Plugin : they should altready be inside the "Plugins" folder.
If prompeted to recompile when openng the unreal project, you might need to do so --also, if there are errors whilst doing so, you might need to close the UE4 project, right click on its icon, select "generate visual studio project", then reopen the project and recompile.

This is a Windows / VS project and it would not fully work on MacOS (mainly due to plugins compatibility issues and certain VR settings).

The UE4 project internal settings should include: enabling of the LeapMotion internal plugin and enabling of all the VR deployments (including GoogleVR).

Note: The OpenFrameworks Sound Engine is a live granular (re)synthesizer built in C++ using openframeworks. It contains 2 folders: the first is the app, the second one contains the custon addons required for the app. You will need to have openframeworks 0.9 for this to run. In openframeworks you need to place the app's foder inside "myApps" or on a directory of the same level. The addons are to be placed in the "addons" direcotry AND you will also need to make sure you have ofxOsc and ofxGui too in there.

The idea is to build an AR - GUI in Unreal that controls the granular synthesizer using OSC.



-- Note: on my macBook, after installing the drivers of my external soundcard on the Windows partition I am using for this project, I can't use any other in/out sound device with the c++ audio engine of this project -- It's not a big problem for me beacuse I wanted to use this sound card anyway, and it may all be because I'm running Win on a bootcamp partition on a Mac, but you may need to check the console when you run the sound engine to check which ins/outs it is looking for.




(cover image found here: https://ibb.co/jAbvkk)
