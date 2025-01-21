#include "VideoElement.h"

VideoElement::VideoElement() {
    offsetX = offsetY = 0;
    isLoaded = false;
    videoIndex = 0;
    isPlaying = false;
    lastPosition = 0;
}

void VideoElement::update() {
    // if(!isPlaying) return;
}


void VideoElement::draw(const vector<ofVideoPlayer>& videos, const vector<ofColor>& colorSwatches, bool isEditMode, int index) {
    if(videoIndex >= videos.size()) return;
    
    const auto& video = videos[videoIndex];
    if(!video.isLoaded()) return;
    
    ofPushMatrix();
    ofTranslate(x + offsetX, y + offsetY);
    
    if(useColorInput && colorSwatches.size() > max(colorIndex1, colorIndex2)) {
        // Get the video frame pixels for our region
        ofPixels videoPixels = video.getPixels();
        ofPixels regionPixels;
        videoPixels.cropTo(regionPixels, 
            sourceRegion.x, sourceRegion.y, 
            sourceRegion.width, sourceRegion.height);
        
        // Process pixels directly like ImageElement does
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
        // Draw normal video segment
        video.getTexture().drawSubsection(0, 0, TILE_SIZE, TILE_SIZE,
                                        sourceRegion.x, sourceRegion.y,
                                        sourceRegion.width, sourceRegion.height);
    }
    
    // Draw gradient overlay if enabled
    if(showGradient && gradientTexture.isAllocated()) {
        gradientTexture.draw(0, 0, TILE_SIZE, TILE_SIZE);
    }
    
    ofPopMatrix();
    
    // Draw index if in edit mode
    if(isEditMode) {
        if(isPrimary()) {
            // Draw primary indicator in red with asterisk
            ofSetColor(255, 0, 0);  // Red color
            ofDrawBitmapStringHighlight(ofToString(index) + " *", x + 5, y + 15, ofColor(255, 0, 0), ofColor(0));
            ofSetColor(255);  // Reset color
        } else {
            // Draw normal index
            ofDrawBitmapStringHighlight(ofToString(index), x + 5, y + 15);
        }
    }
}

void VideoElement::setVideoRegion(size_t index, const ofRectangle& region) {
    videoIndex = index;
    sourceRegion = region;
    isPlaying = true;
    isLoaded = true;
}

