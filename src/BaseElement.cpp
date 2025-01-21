#include "BaseElement.h"

// Initialize static members
ofTexture BaseElement::gradientTexture;
bool BaseElement::showGradient = true;

void BaseElement::loadGradientTexture() {
    ofImage gradientImage;
    if(gradientImage.load("gradient.png")) {
        gradientTexture = gradientImage.getTexture();
        ofLog() << "Loaded gradient texture";
    } else {
        ofLog() << "Failed to load gradient texture";
    }
} 