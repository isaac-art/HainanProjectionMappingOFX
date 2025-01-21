#include "ImageElement.h"

ImageElement::ImageElement() {
    offsetX = offsetY = 0;
    isLoaded = false;
    imageIndex = 0;
}

void ImageElement::draw(const vector<ofImage>& images, const vector<ofColor>& colorSwatches, 
                       bool showGui, size_t tileIndex) const {
    if(isLoaded && imageIndex < images.size()) {
        const auto& image = images[imageIndex];
        
        if(useColorInput && !isPrimaryElement && colorSwatches.size() > max(colorIndex1, colorIndex2)) {
            // Get the image region
            ofPixels imagePixels = image.getPixels();
            ofPixels pixels;
            pixels.allocate(sourceRegion.width, sourceRegion.height, OF_PIXELS_RGB);
            
            // Copy the region manually
            for(int y = 0; y < sourceRegion.height; y++) {
                for(int x = 0; x < sourceRegion.width; x++) {
                    ofColor color = imagePixels.getColor(
                        x + sourceRegion.x, 
                        y + sourceRegion.y
                    );
                    pixels.setColor(x, y, color);
                }
            }
            
            // Allocate OpenCV images if needed
            if(!isCvImageAllocated || 
               cvImage.getWidth() != sourceRegion.width || 
               cvImage.getHeight() != sourceRegion.height) {
                cvImage.clear();
                grayImage.clear();
                cvImage.allocate(sourceRegion.width, sourceRegion.height);
                grayImage.allocate(sourceRegion.width, sourceRegion.height);
                isCvImageAllocated = true;
            }
            
            // Convert to grayscale using OpenCV
            cvImage.setFromPixels(pixels);
            grayImage = cvImage;
            
            // Get grayscale pixels
            ofPixels& grayPixels = grayImage.getPixels();
            ofPixels coloredPixels;
            coloredPixels.allocate(sourceRegion.width, sourceRegion.height, OF_PIXELS_RGB);
            
            // Map grayscale values to colors
            for(size_t y = 0; y < sourceRegion.height; y++) {
                for(size_t x = 0; x < sourceRegion.width; x++) {
                    float brightness = grayPixels.getColor(x, y).getBrightness() / 255.0f;
                    ofColor mappedColor = colorSwatches[colorIndex1].getLerped(
                        colorSwatches[colorIndex2], brightness);
                    coloredPixels.setColor(x, y, mappedColor);
                }
            }
            
            // Draw the processed image
            ofTexture tex;
            tex.loadData(coloredPixels);
            tex.draw(x + offsetX, y + offsetY, TILE_SIZE, TILE_SIZE);
            
        } else {
            // Normal drawing without color replacement
            image.getTexture().drawSubsection(
                x + offsetX, y + offsetY, 
                TILE_SIZE, TILE_SIZE,
                sourceRegion.x, sourceRegion.y,
                sourceRegion.width, sourceRegion.height
            );
        }
        
        // Draw gradient overlay if enabled
        if(showGradient && gradientTexture.isAllocated()) {
            ofPushStyle();
            ofEnableAlphaBlending();
            ofSetColor(255);
            gradientTexture.draw(x + offsetX, y + offsetY, TILE_SIZE, TILE_SIZE);
            ofPopStyle();
        }
        
        // Draw tile index if GUI is shown
        if(showGui) {
            ofPushStyle();
            float padding = 4;
            string indexStr = ofToString(tileIndex);
            if(isPrimaryElement) indexStr += "*";
            
            float textWidth = 20;
            float textHeight = 15;
            
            ofSetColor(255);
            ofDrawRectangle(x + offsetX, y + offsetY, 
                          textWidth + padding * 2, textHeight + padding * 2);
            
            ofSetColor(0);
            if(isPrimaryElement) ofSetColor(255, 0, 0);
            ofDrawBitmapString(indexStr, 
                             x + offsetX + padding, 
                             y + offsetY + textHeight);
            ofPopStyle();
        }
    } else {
        ofSetColor(40);
        ofDrawRectangle(x + offsetX, y + offsetY, TILE_SIZE, TILE_SIZE);
        ofSetColor(255);
    }
}

void ImageElement::setImageRegion(size_t index, const ofRectangle& region) {
    imageIndex = index;
    sourceRegion = region;
    isLoaded = true;
} 