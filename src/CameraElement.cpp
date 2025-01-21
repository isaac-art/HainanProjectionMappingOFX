#include "CameraElement.h"

CameraElement::CameraElement() {
    offsetX = offsetY = 0;
    isLoaded = false;
    cameraIndex = 0;
    isActive = false;
}

void CameraElement::update() {
    // Camera updates are handled by ofVideoGrabber in the main app
}

void CameraElement::draw(const vector<ofVideoGrabber>& cameras, const vector<ofColor>& colorSwatches, bool isEditMode, int index) {
    if(cameraIndex >= cameras.size()) return;
    
    const auto& camera = cameras[cameraIndex];
    if(!camera.isInitialized()) return;
    
    ofPushMatrix();
    ofTranslate(x + offsetX, y + offsetY);
    
    if(useColorInput && colorSwatches.size() > max(colorIndex1, colorIndex2)) {
        // Get the camera frame pixels for our region
        ofPixels cameraPixels = camera.getPixels();
        ofPixels regionPixels;
        cameraPixels.cropTo(regionPixels, 
            sourceRegion.x, sourceRegion.y, 
            sourceRegion.width, sourceRegion.height);
        
        // Process pixels directly like VideoElement
        for(size_t i = 0; i < regionPixels.size(); i += regionPixels.getNumChannels()) {
            float r = regionPixels[i];
            float g = regionPixels[i + 1];
            float b = regionPixels[i + 2];
            
            // Calculate brightness using perceived luminance weights
            float brightness = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
            
            // Interpolate between colors based on brightness
            ofColor result = colorSwatches[colorIndex1].getLerped(colorSwatches[colorIndex2], brightness);
            
            regionPixels[i] = result.r;
            regionPixels[i + 1] = result.g;
            regionPixels[i + 2] = result.b;
        }
        
        ofTexture tex;
        tex.loadData(regionPixels);
        tex.draw(0, 0, TILE_SIZE, TILE_SIZE);
    } else {
        // Draw normal camera segment
        camera.getTexture().drawSubsection(0, 0, TILE_SIZE, TILE_SIZE,
                                         sourceRegion.x, sourceRegion.y,
                                         sourceRegion.width, sourceRegion.height);
    }
    
    // Draw gradient overlay if enabled
    if(showGradient && gradientTexture.isAllocated()) {
        ofSetColor(255, 255, 255, 128);
        gradientTexture.draw(0, 0, TILE_SIZE, TILE_SIZE);
        ofSetColor(255, 255, 255, 255);
    }
    
    ofPopMatrix();
    
    // Draw index if in edit mode
    if(isEditMode) {
        ofDrawBitmapStringHighlight(ofToString(index), x + 5, y + 15);
    }
}

void CameraElement::setCameraRegion(size_t index, const ofRectangle& region) {
    cameraIndex = index;
    sourceRegion = region;
    isActive = true;
    isLoaded = true;
} 