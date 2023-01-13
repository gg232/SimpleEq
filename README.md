# SimpleEq
This is currently a simple linear EQ plugin with a high-pass, low-pass, and cut filter. It was written with the [JUCE framework](https://github.com/juce-framework/JUCE) based on [Matkat's YouTube tutorial](https://www.youtube.com/watch?v=i_Iq4_Kd7Rc).

My goal is to write up a saturating EQ by adding a waveshaper that will add gentle tube-like distortion after the EQ. I would also like to add more bands, and possibly the ability to add an arbitrary number of bands (ala ReaEQ) pre- or -post saturation.

I have compiled it on Windows. The binary is not (currently) contained in this repo. To compile it on your system, open the .jucer file in the Projucer and export the project to your editor of choice.

# Scope
This plugin is, as of writing this, a personal project with the intention of teaching myself how to develop VST plugins. Although subsequent iterations will use JUCE classes to do the "math", I hope that the final project will use purpose built "homebrew" DSP algorithms. I intend for JUCE to handle the GUI stuff.

# Warranty
There is none. I recommend you do not use this (yet) on professional projects.

# License
When it is released, it will be under one of the GPL's. I'll flesh this out later.
