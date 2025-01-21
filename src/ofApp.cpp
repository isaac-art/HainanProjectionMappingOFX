#include "ofApp.h"
#include <memory>

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    selectedTile = 0;
    adjustmentSpeed = 1.0;
    isDragging = false;
    isGroupSelected = false;
    showGui = true;
    
    
    // Load the gradient texture
    VideoElement::loadGradientTexture();
    
    setupGui();
    setupOsc();
    loadLayout();

    lastSwatchUpdate = ofGetElapsedTimef();
}

void ofApp::setupGui() {
    // Main controls panel
    gui.setup("Video Grid Controls");
    gui.add(newLayoutBtn.setup("New Layout"));
    gui.add(currentLayoutLabel.setup("Current Layout", ""));
    gui.add(saveLayoutBtn.setup("Save New Layout"));
    gui.add(saveChangesBtn.setup("Save Changes"));
    gui.add(loadLayoutBtn.setup("Load Selected Layout"));
    gui.add(addVideoBtn.setup("Add New Video"));
    gui.add(addImageBtn.setup("Add New Image"));
    gui.add(gradientToggle.setup("Show Gradient", true));
    
    // Add primary video selection
    gui.add(primaryVideoLabel.setup("Primary Video", ""));
    
    // Fix the parameter setup
    primaryVideoIndex.addListener(this, &ofApp::onPrimaryVideoChanged);
    primaryVideoIndex.set("Set Primary", -1);  // Simplified parameter setup
    primaryVideoIndex.setMin(-1);
    primaryVideoIndex.setMax(0);
    gui.add(primaryVideoIndex);
    
    gui.setPosition(10, 10);
    
    // Info panel
    infoPanel.setup("Selected Tile Info");
    infoPanel.add(tileIndexLabel.setup("Tile Index", ""));
    infoPanel.add(videoPathLabel.setup("Video Path", ""));
    infoPanel.add(changeVideoBtn.setup("Change Video"));
    infoPanel.add(tilePosLabel.setup("Position", ""));
    infoPanel.add(tileSizeLabel.setup("Source Region", ""));
    
    
    infoPanel.setPosition(gui.getPosition().x, gui.getPosition().y + gui.getHeight() + 100);
    
    // Add listeners
    saveLayoutBtn.addListener(this, &ofApp::saveLayout);
    saveChangesBtn.addListener(this, &ofApp::saveCurrentLayout);
    loadLayoutBtn.addListener(this, &ofApp::loadLayout);
    addVideoBtn.addListener(this, &ofApp::loadNewVideo);
    changeVideoBtn.addListener(this, &ofApp::changeSelectedVideo);
   
    gradientToggle.addListener(this, &ofApp::onGradientToggled);
    addImageBtn.addListener(this, &ofApp::loadNewImage);
    newLayoutBtn.addListener(this, &ofApp::createNewLayout);
    
    selectedLayout = 0;
    refreshLayoutList();
    
    // Setup swatch panel
    swatchPanel.setup("Primary Video Colors");
    swatchPanel.setPosition(gui.getPosition().x + gui.getWidth() + 10, gui.getPosition().y);
    
    // Initialize color swatches
    colorSwatches.resize(NUM_SWATCHES);
    
    // Add color input toggle to info panel
    infoPanel.add(colorInputToggle.setup("Use Color Input", false));
    
    // Correct way to add parameters to GUI
    color1Index.set("Color 1", 0, 0, NUM_SWATCHES-1);
    color2Index.set("Color 2", 1, 0, NUM_SWATCHES-1);
    infoPanel.add(color1Index);
    infoPanel.add(color2Index);
    
    color1Index.addListener(this, &ofApp::onColor1Changed);
    color2Index.addListener(this, &ofApp::onColor2Changed);
    
    gui.add(addCameraBtn.setup("Add Camera Tile"));
    addCameraBtn.addListener(this, &ofApp::addCameraTile);

    // Setup video preview panel
    setupVideoPreviewPanel();
}

void ofApp::refreshLayoutList() {
    layoutFiles.clear();
    
    ofDirectory dir("layouts");
    if(!dir.exists()) {
        dir.create();
        return;
    }
    
    dir.listDir();
    dir.sort();
    
    for(const auto& file : dir.getFiles()) {
        if(file.getExtension() == "json") {
            layoutFiles.push_back(file.getBaseName());
        }
    }
    
    // Update current layout label
    if(!layoutFiles.empty() && selectedLayout < layoutFiles.size()) {
        currentLayoutLabel = layoutFiles[selectedLayout];
    } else {
        currentLayoutLabel = "No layouts";
        selectedLayout = 0;
    }
}

string ofApp::generateLayoutName() {
    // Generate datetime string
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "layout_" << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S");
    return ss.str();
}

//--------------------------------------------------------------
void ofApp::update(){
    updateOsc();
    
    // Update all videos based on their playback settings
    for(size_t i = 0; i < videos.size(); i++) {
        auto& video = videos[i];
        if(video.isLoaded()) {
            if(i < videoPlaybackSettings.size()) {
                const auto& settings = videoPlaybackSettings[i];
                APlaybackMode mode = std::get<0>(settings);
                
                if(mode == APlaybackMode::LOOP) {
                    // Normal looping behavior - ensure video is playing
                    video.setSpeed(1);
                    if(!video.isPlaying()) {
                        video.play();
                    }
                } else if(mode == APlaybackMode::OSC_PLAYBACK) {
                    
                    // OSC controlled playback - pause and seek based on OSC input
                    // Make sure video is playing to allow frame updates
                    if(!video.isPlaying()) {
                        video.play();
                    }
                    
                    // Get the appropriate OSC value based on input type
                    AOscInputType oscType = std::get<1>(settings);
                    float value = 0;
                    switch(oscType) {
                        case AOscInputType::YAW:
                            value = yawValue;
                            break;
                        case AOscInputType::PITCH:
                            value = pitchValue;
                            break;
                        case AOscInputType::ROLL:
                            value = rollValue;
                            break;
                    }
                    
                    float position = ofMap(value, -1, 1, -10, 10, true);
                    // ofLog() << "Setting osc video position to: " << position * 100 << "%";
                    // video.setPosition(position);
                    video.setSpeed(position);
                    // video.update();  // Force immediate frame update
                }
            }
            video.update();
        }
    }
    
    // Check if primary video just started playing
    for(const auto& tile : tiles) {
        if(tile.isPrimary()) {
            if(tile.videoIndex < videos.size()) {
                const auto& video = videos[tile.videoIndex];
                if(video.getCurrentFrame() == 20) {  // 20th frame as some start black or white
                    needsSwatchUpdate = true;
                }
            }
            break;
        }
    }

    // Check if we need to update swatches periodically
    
    float currentTime = ofGetElapsedTimef();
    if(currentTime - lastSwatchUpdate > 5.0) {  // Every 10 seconds
        needsSwatchUpdate = true;
        lastSwatchUpdate = currentTime;
    }
    
    // Update swatches if needed
    if(needsSwatchUpdate) {
        updateColorSwatchesFromPrimary();
        needsSwatchUpdate = false;
    }
    
    // Update tiles
    for(auto& tile : tiles) {
        tile.update();
    }
    
    for(auto& camera : cameras) {
        camera.update();
    }


}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    
    // Draw video tiles
    for(size_t i = 0; i < tiles.size(); i++) {
        tiles[i].draw(videos, colorSwatches, showGui, i);
        
        // Show selection highlights in edit mode
        if(showGui && ((isGroupSelected && find(selectedTiles.begin(), selectedTiles.end(), i) != selectedTiles.end()) ||
           (!isGroupSelected && i == selectedTile))) {
            ofPushStyle();
            ofNoFill();
            ofSetColor(255, 0, 0);
            ofDrawRectangle(
                tiles[i].x + tiles[i].offsetX, 
                tiles[i].y + tiles[i].offsetY, 
                VideoElement::TILE_SIZE, 
                VideoElement::TILE_SIZE
            );
            ofFill();
            ofPopStyle();
        }
    }
    
    // Draw image tiles
    for(size_t i = 0; i < imageTiles.size(); i++) {
        imageTiles[i].draw(images, colorSwatches, showGui, i + tiles.size());  // Offset index for proper numbering
        
        // Show selection highlights in edit mode
        if(showGui && ((isGroupSelected && find(selectedTiles.begin(), selectedTiles.end(), i + tiles.size()) != selectedTiles.end()) ||
           (!isGroupSelected && i + tiles.size() == selectedTile))) {
            ofPushStyle();
            ofNoFill();
            ofSetColor(255, 0, 0);
            ofDrawRectangle(
                imageTiles[i].x + imageTiles[i].offsetX, 
                imageTiles[i].y + imageTiles[i].offsetY, 
                ImageElement::TILE_SIZE, 
                ImageElement::TILE_SIZE
            );
            ofFill();
            ofPopStyle();
        }
    }
    
    // Draw camera tiles
    for(size_t i = 0; i < cameraTiles.size(); i++) {
        cameraTiles[i].draw(cameras, colorSwatches, showGui, i + tiles.size() + imageTiles.size());
        
        // Show selection highlights in edit mode
        if(showGui && ((isGroupSelected && 
            find(selectedTiles.begin(), selectedTiles.end(), i + tiles.size() + imageTiles.size()) != selectedTiles.end()) ||
            (!isGroupSelected && i + tiles.size() + imageTiles.size() == selectedTile))) {
            ofPushStyle();
            ofNoFill();
            ofSetColor(255, 0, 0);
            ofDrawRectangle(
                cameraTiles[i].x + cameraTiles[i].offsetX,
                cameraTiles[i].y + cameraTiles[i].offsetY,
                CameraElement::TILE_SIZE,
                CameraElement::TILE_SIZE
            );
            ofFill();
            ofPopStyle();
        }
    }
    
    if(showGui) {
        updateInfoPanel();
        updateVideoPreviewPanel();
        gui.draw();
        infoPanel.draw();
        if(showVideoPreview) {  
            videoPreviewPanel.draw();
            // drawVideoPreview();  // Comment out video preview
        }
        drawColorSwatches();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    float dx = 0, dy = 0;
    
    switch(key) {
        case 'g':
            if(showGui) {
                // Save current layout before hiding GUI
                saveCurrentLayout();
            }
            showGui = !showGui;
            // Clear selection when entering locked mode
            if(!showGui) {
                isGroupSelected = false;
                selectedTiles.clear();
                selectedTile = 0;
            }
            break;
        // Layout navigation (always allowed)
        case OF_KEY_UP:
            if(ofGetKeyPressed(OF_KEY_ALT)) {
                previousLayout();
                loadLayout();
                return;
            }
            // Fall through to edit mode check
            
        case OF_KEY_DOWN:
            if(ofGetKeyPressed(OF_KEY_ALT)) {
                nextLayout();
                loadLayout();
                return;
            }
            // Fall through to edit mode check
    }
    
    // Only allow editing operations when GUI is visible
    if(!isEditMode()) return;
    
    switch(key) {
        case OF_KEY_UP:
            selectedTile = max(0, selectedTile - 1);
            if(!ofGetKeyPressed(OF_KEY_SHIFT)) {
                isGroupSelected = false;
                selectedTiles.clear();
            }
            break;
            
        case OF_KEY_DOWN:
            selectedTile = min((int)tiles.size() - 1, selectedTile + 1);
            if(!ofGetKeyPressed(OF_KEY_SHIFT)) {
                isGroupSelected = false;
                selectedTiles.clear();
            }
            break;
            
        case OF_KEY_LEFT:
            selectedTile = max(0, selectedTile - 1);
            if(!ofGetKeyPressed(OF_KEY_SHIFT)) {
                isGroupSelected = false;
                selectedTiles.clear();
            }
            break;
            
        case OF_KEY_RIGHT:
            selectedTile = min((int)tiles.size() - 1, selectedTile + 1);
            if(!ofGetKeyPressed(OF_KEY_SHIFT)) {
                isGroupSelected = false;
                selectedTiles.clear();
            }
            break;
            
        // Adjust position
        case 'w': 
            dy = -adjustmentSpeed;
            moveSelectedTiles(0, dy);
            break;
            
        case 's':
            dy = adjustmentSpeed;
            moveSelectedTiles(0, dy);
            break;
            
        case 'a':
            dx = -adjustmentSpeed;
            moveSelectedTiles(dx, 0);
            break;
            
        case 'd':
            dx = adjustmentSpeed;
            moveSelectedTiles(dx, 0);
            break;
            
        // Delete selected tile
        case 'x':
            deleteTile(selectedTile);
            break;
            
            
        // Undo
        case 'z':
            if(ofGetKeyPressed(OF_KEY_COMMAND) || ofGetKeyPressed(OF_KEY_CONTROL)) {
                undo();
            }
            break;
            
        case 'c':  // Press 'c' to update color swatches
            if(isEditMode()) {
                updateColorSwatchesFromPrimary();
            }
            break;
            
        case 'y':  // Add grid alignment
            alignTilesToGrid();
            break;
    }
    

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    if(!isEditMode()) return;  // Ignore mouse input in locked mode
    
    int clickedTile = findTileUnderMouse(x, y);
    if(clickedTile >= 0) {
        isDragging = true;
        dragStartPos.set(x, y);
        
        // Alt+Shift+Click: Select all tiles
        if(ofGetKeyPressed(OF_KEY_ALT) && ofGetKeyPressed(OF_KEY_SHIFT)) {
            selectedTiles.clear();
            
            // Add all video tiles
            for(size_t i = 0; i < tiles.size(); i++) {
                selectedTiles.push_back(i);
            }
            
            // Add all image tiles
            for(size_t i = 0; i < imageTiles.size(); i++) {
                selectedTiles.push_back(i + tiles.size());
            }
            
            // Add all camera tiles
            for(size_t i = 0; i < cameraTiles.size(); i++) {
                selectedTiles.push_back(i + tiles.size() + imageTiles.size());
            }
            
            isGroupSelected = true;
            selectedTile = clickedTile;  // Keep track of clicked tile
        }
        // Regular Shift+Click: Select tiles from same source
        else if(ofGetKeyPressed(OF_KEY_SHIFT)) {
            selectTilesFromSameSource(clickedTile);
        }
        // Alt+Click: Add/remove individual tile to/from selection
        else if(ofGetKeyPressed(OF_KEY_ALT)) {
            auto it = find(selectedTiles.begin(), selectedTiles.end(), clickedTile);
            if(it != selectedTiles.end()) {
                // Remove from selection if already selected
                selectedTiles.erase(it);
                if(selectedTiles.empty()) {
                    isGroupSelected = false;
                }
            } else {
                // Add to selection
                if(!isGroupSelected) {
                    // If this is the first alt+click, add the currently selected tile first
                    if(selectedTile >= 0 && selectedTiles.empty()) {
                        selectedTiles.push_back(selectedTile);
                    }
                    isGroupSelected = true;
                }
                selectedTiles.push_back(clickedTile);
            }
            selectedTile = clickedTile;  // Update current tile
        } else {
            // Single tile selection
            selectedTile = clickedTile;
            selectedTiles.clear();
            selectedTiles.push_back(clickedTile);
            isGroupSelected = false;
        }
        
        // Store initial positions of selected tiles
        tileRelativePositions.clear();
        for(int index : selectedTiles) {
            ofPoint pos;
            size_t videoTilesEnd = tiles.size();
            size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
            
            if(index < videoTilesEnd) {
                pos.set(tiles[index].x, tiles[index].y);
            } else if(index < imageTilesEnd) {
                int imageIndex = index - videoTilesEnd;
                pos.set(imageTiles[imageIndex].x, imageTiles[imageIndex].y);
            } else {
                int cameraIndex = index - imageTilesEnd;
                pos.set(cameraTiles[cameraIndex].x, cameraTiles[cameraIndex].y);
            }
            tileRelativePositions.push_back(pos);
        }
    } else {
        // Clicked empty space - clear selection unless holding Alt
        if(!ofGetKeyPressed(OF_KEY_ALT)) {
            selectedTile = -1;
            selectedTiles.clear();
            isGroupSelected = false;
        }
        isDragging = false;
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
    if(!isEditMode() || !isDragging) return;
    
    float dx = x - dragStartPos.x;
    float dy = y - dragStartPos.y;
    
    // Move all selected tiles (either single tile or group)
    for(size_t i = 0; i < selectedTiles.size(); i++) {
        int index = selectedTiles[i];
        ofPoint startPos = tileRelativePositions[i];
        
        size_t videoTilesEnd = tiles.size();
        size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
        
        if(index < videoTilesEnd) {
            tiles[index].x = startPos.x + dx;
            tiles[index].y = startPos.y + dy;
        } else if(index < imageTilesEnd) {
            int imageIndex = index - videoTilesEnd;
            imageTiles[imageIndex].x = startPos.x + dx;
            imageTiles[imageIndex].y = startPos.y + dy;
        } else {
            int cameraIndex = index - imageTilesEnd;
            cameraTiles[cameraIndex].x = startPos.x + dx;
            cameraTiles[cameraIndex].y = startPos.y + dy;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    if(isDragging) {
        // Record the move for undo
        vector<int> movedTiles = isGroupSelected ? selectedTiles : vector<int>{selectedTile};
        recordTileMove(movedTiles, isGroupSelected);
        
        // Save the current layout
        saveCurrentLayout();
    }
    isDragging = false;
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

void ofApp::loadVideoAsTiles(const string& path) {
    // Create new video player
    videos.emplace_back();
    size_t videoIndex = videos.size() - 1;
    ofVideoPlayer& currentVideo = videos.back();
    
    // Load new video
    if(!currentVideo.load(path)) {
        ofLogError() << "Failed to load video: " << path;
        videos.pop_back();
        return;
    }

    // Add default playback settings for the new video (LOOP mode)
    videoPlaybackSettings.push_back(std::make_tuple(APlaybackMode::LOOP, AOscInputType::YAW));
    
    currentVideo.play();
    
    int videoWidth = currentVideo.getWidth();
    int videoHeight = currentVideo.getHeight();
    
    // Calculate number of tiles needed
    int tilesWide = ceil(float(videoWidth) / VideoElement::TILE_SIZE);
    int tilesHigh = ceil(float(videoHeight) / VideoElement::TILE_SIZE);
    
    // Calculate starting position for this set of tiles
    float startX = 10;
    float startY = 10;
    
    // Create tiles
    for(int y = 0; y < tilesHigh; y++) {
        for(int x = 0; x < tilesWide; x++) {
            VideoElement tile;
            
            // Position tile
            float tileX = startX + x * VideoElement::TILE_SIZE;
            float tileY = startY + y * VideoElement::TILE_SIZE;
            tile.setup(tileX, tileY);
            
            // Calculate source region for this tile
            ofRectangle region(
                x * VideoElement::TILE_SIZE,
                y * VideoElement::TILE_SIZE,
                min(VideoElement::TILE_SIZE, videoWidth - x * VideoElement::TILE_SIZE),
                min(VideoElement::TILE_SIZE, videoHeight - y * VideoElement::TILE_SIZE)
            );
            
            tile.setVideoRegion(videoIndex, region);
            tile.setPath(path);  // Make sure to set the path
            tiles.push_back(tile);
        }
    }
}

void ofApp::deleteTile(int index) {
    if(index >= 0) {
        size_t videoTilesEnd = tiles.size();
        size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
        size_t cameraTilesEnd = imageTilesEnd + cameraTiles.size();
        
        if(index < videoTilesEnd) {
            // Delete video tile
            tiles.erase(tiles.begin() + index);
        } else if(index < imageTilesEnd) {
            // Delete image tile
            int imageIndex = index - videoTilesEnd;
            imageTiles.erase(imageTiles.begin() + imageIndex);
        } else if(index < cameraTilesEnd) {
            // Delete camera tile
            int cameraIndex = index - imageTilesEnd;
            cameraTiles.erase(cameraTiles.begin() + cameraIndex);
        }
        
        // Update selected tile index
        if(selectedTile >= index) {
            selectedTile = max(0, (int)(tiles.size() + imageTiles.size() + cameraTiles.size()) - 1);
        }
        
        // Clear group selection if the deleted tile was part of it
        if(isGroupSelected) {
            auto it = find(selectedTiles.begin(), selectedTiles.end(), index);
            if(it != selectedTiles.end()) {
                selectedTiles.erase(it);
                if(selectedTiles.empty()) {
                    isGroupSelected = false;
                }
            }
        }
        
        // Save the current layout after deletion
        saveCurrentLayout();
    }
}

int ofApp::findTileUnderMouse(int x, int y) {
    // Check camera tiles first (they're drawn on top)
    for(int i = cameraTiles.size() - 1; i >= 0; i--) {
        const auto& tile = cameraTiles[i];
        float tileX = tile.x + tile.offsetX;
        float tileY = tile.y + tile.offsetY;
        
        if(x >= tileX && x < tileX + CameraElement::TILE_SIZE &&
           y >= tileY && y < tileY + CameraElement::TILE_SIZE) {
            return i + tiles.size() + imageTiles.size();  // Return offset index for camera tiles
        }
    }
    
    // Check image tiles
    for(int i = imageTiles.size() - 1; i >= 0; i--) {
        const auto& tile = imageTiles[i];
        float tileX = tile.x + tile.offsetX;
        float tileY = tile.y + tile.offsetY;
        
        if(x >= tileX && x < tileX + ImageElement::TILE_SIZE &&
           y >= tileY && y < tileY + ImageElement::TILE_SIZE) {
            return i + tiles.size();  // Return offset index for image tiles
        }
    }
    
    // Check video tiles
    for(int i = tiles.size() - 1; i >= 0; i--) {
        const auto& tile = tiles[i];
        float tileX = tile.x + tile.offsetX;
        float tileY = tile.y + tile.offsetY;
        
        if(x >= tileX && x < tileX + VideoElement::TILE_SIZE &&
           y >= tileY && y < tileY + VideoElement::TILE_SIZE) {
            return i;
        }
    }
    
    return -1;
}

void ofApp::selectTilesFromSameSource(int tileIndex) {
    selectedTiles.clear();
    
    size_t videoTilesEnd = tiles.size();
    size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
    size_t cameraTilesEnd = imageTilesEnd + cameraTiles.size();
    
    if(tileIndex < videoTilesEnd) {
        // Video tile selected
        size_t videoIndex = tiles[tileIndex].videoIndex;
        for(int i = 0; i < tiles.size(); i++) {
            if(tiles[i].videoIndex == videoIndex) {
                selectedTiles.push_back(i);
            }
        }
    } else if(tileIndex < imageTilesEnd) {
        // Image tile selected
        int imageIndex = tileIndex - videoTilesEnd;
        size_t sourceImageIndex = imageTiles[imageIndex].imageIndex;
        for(int i = 0; i < imageTiles.size(); i++) {
            if(imageTiles[i].imageIndex == sourceImageIndex) {
                selectedTiles.push_back(i + videoTilesEnd);
            }
        }
    } else if(tileIndex < cameraTilesEnd) {
        // Camera tile selected
        int cameraIndex = tileIndex - imageTilesEnd;
        size_t sourceCameraIndex = cameraTiles[cameraIndex].cameraIndex;
        for(int i = 0; i < cameraTiles.size(); i++) {
            if(cameraTiles[i].cameraIndex == sourceCameraIndex) {
                selectedTiles.push_back(i + imageTilesEnd);
            }
        }
    }
    
    isGroupSelected = true;
    selectedTile = tileIndex;
}

void ofApp::moveSelectedTiles(float dx, float dy) {
    vector<int> movedTiles;
    
    if(isGroupSelected) {
        movedTiles = selectedTiles;
        for(int index : selectedTiles) {
            size_t videoTilesEnd = tiles.size();
            size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
            
            if(index < videoTilesEnd) {
                // Move video tile
                tiles[index].x += dx;
                tiles[index].y += dy;
            } else if(index < imageTilesEnd) {
                // Move image tile
                int imageIndex = index - videoTilesEnd;
                imageTiles[imageIndex].x += dx;
                imageTiles[imageIndex].y += dy;
            } else {
                // Move camera tile
                int cameraIndex = index - imageTilesEnd;
                cameraTiles[cameraIndex].x += dx;
                cameraTiles[cameraIndex].y += dy;
            }
        }
    } else if(selectedTile >= 0) {
        movedTiles = {selectedTile};
        size_t videoTilesEnd = tiles.size();
        size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
        
        if(selectedTile < videoTilesEnd) {
            // Move video tile
            tiles[selectedTile].x += dx;
            tiles[selectedTile].y += dy;
        } else if(selectedTile < imageTilesEnd) {
            // Move image tile
            int imageIndex = selectedTile - videoTilesEnd;
            imageTiles[imageIndex].x += dx;
            imageTiles[imageIndex].y += dy;
        } else {
            // Move camera tile
            int cameraIndex = selectedTile - imageTilesEnd;
            cameraTiles[cameraIndex].x += dx;
            cameraTiles[cameraIndex].y += dy;
        }
    }
    
    // Record the move for undo if using keyboard
    if(dx != 0 || dy != 0) {
        recordTileMove(movedTiles, isGroupSelected);
    }
}

void ofApp::recordTileMove(const vector<int>& tileIndices, bool isGroup) {
    MoveAction action;
    action.isGroup = isGroup;
    
    for(int index : tileIndices) {
        if(index >= 0) {
            TileMove move;
            move.tileIndex = index;
            
            if(index < tiles.size()) {
                // Record video tile position
                move.newX = tiles[index].x;
                move.newY = tiles[index].y;
                move.oldX = tileStartPos.x - tiles[index].offsetX;
                move.oldY = tileStartPos.y - tiles[index].offsetY;
            } else {
                // Record image tile position
                int imageIndex = index - tiles.size();
                if(imageIndex < imageTiles.size()) {
                    move.newX = imageTiles[imageIndex].x;
                    move.newY = imageTiles[imageIndex].y;
                    move.oldX = tileStartPos.x - imageTiles[imageIndex].offsetX;
                    move.oldY = tileStartPos.y - imageTiles[imageIndex].offsetY;
                }
            }
            action.moves.push_back(move);
        }
    }
    
    if(!action.moves.empty()) {
        undoHistory.push_front(action);
        if(undoHistory.size() > MAX_UNDO_HISTORY) {
            undoHistory.pop_back();
        }
    }
}

void ofApp::undo() {
    if(undoHistory.empty()) return;
    
    MoveAction& action = undoHistory.front();
    for(const auto& move : action.moves) {
        if(move.tileIndex < tiles.size()) {
            // Undo video tile move
            tiles[move.tileIndex].x = move.oldX;
            tiles[move.tileIndex].y = move.oldY;
        } else {
            // Undo image tile move
            int imageIndex = move.tileIndex - tiles.size();
            if(imageIndex < imageTiles.size()) {
                imageTiles[imageIndex].x = move.oldX;
                imageTiles[imageIndex].y = move.oldY;
            }
        }
    }
    
    undoHistory.pop_front();
}

string ofApp::getLayoutPath(const string& name) {
    return "layouts/" + name + ".json";
}

void ofApp::saveLayout() {
    string layoutName = generateLayoutName();
    ofJson layout;
    layout["videos"] = nlohmann::json::array();
    
    // Save video paths and their tiles
    map<size_t, string> videoPathMap;  // Map to store unique video paths
    
    // First, collect all video paths
    for(const auto& tile : tiles) {
        if(tile.videoIndex < videos.size()) {
            if(videoPathMap.find(tile.videoIndex) == videoPathMap.end()) {
                videoPathMap[tile.videoIndex] = videos[tile.videoIndex].getMoviePath();
            }
        }
    }
    
    // Save video paths
    for(const auto& pair : videoPathMap) {
        ofJson videoData;
        videoData["index"] = pair.first;
        videoData["path"] = pair.second;
        layout["videos"].push_back(videoData);
    }
    
    // Save tiles
    layout["tiles"] = nlohmann::json::array();
    for(const auto& tile : tiles) {
        ofJson tileData;
        tileData["videoIndex"] = tile.videoIndex;
        tileData["x"] = tile.x;
        tileData["y"] = tile.y;
        tileData["offsetX"] = tile.offsetX;
        tileData["offsetY"] = tile.offsetY;
        tileData["sourceRegion"] = {
            {"x", tile.sourceRegion.x},
            {"y", tile.sourceRegion.y},
            {"width", tile.sourceRegion.width},
            {"height", tile.sourceRegion.height}
        };
        tileData["isPrimary"] = tile.isPrimary();
        tileData["useColorInput"] = tile.hasColorInput();
        tileData["colorIndex1"] = tile.getColorIndex1();
        tileData["colorIndex2"] = tile.getColorIndex2();
        layout["tiles"].push_back(tileData);
    }
    
    // Save camera tiles
    layout["cameraTiles"] = nlohmann::json::array();
    for(const auto& tile : cameraTiles) {
        ofJson tileData;
        tileData["cameraIndex"] = tile.cameraIndex;
        tileData["x"] = tile.x;
        tileData["y"] = tile.y;
        tileData["offsetX"] = tile.offsetX;
        tileData["offsetY"] = tile.offsetY;
        tileData["sourceRegion"] = {
            {"x", tile.sourceRegion.x},
            {"y", tile.sourceRegion.y},
            {"width", tile.sourceRegion.width},
            {"height", tile.sourceRegion.height}
        };
        tileData["isPrimary"] = tile.isPrimary();
        tileData["useColorInput"] = tile.hasColorInput();
        tileData["colorIndex1"] = tile.getColorIndex1();
        tileData["colorIndex2"] = tile.getColorIndex2();
        layout["cameraTiles"].push_back(tileData);
    }
    
    // Create layouts directory if it doesn't exist
    ofDirectory dir("layouts");
    if(!dir.exists()) {
        dir.create();
    }
    
    // Save to file with datetime name
    string path = getLayoutPath(layoutName);
    ofSavePrettyJson(path, layout);
    ofLog() << "Layout saved to: " << path;
    
    // Refresh layout list
    refreshLayoutList();
    
    // Select the newly created layout
    for(size_t i = 0; i < layoutFiles.size(); i++) {
        if(layoutFiles[i] == layoutName) {
            selectedLayout = i;
            currentLayoutLabel = layoutName;
            break;
        }
    }
}

void ofApp::loadLayout() {
    if(layoutFiles.empty() || selectedLayout >= layoutFiles.size()) return;
    
    string path = getLayoutPath(layoutFiles[selectedLayout]);
    ofJson layout = ofLoadJson(path);
    
    // Clear existing elements
    tiles.clear();
    imageTiles.clear();
    cameraTiles.clear();
    videos.clear();
    images.clear();
    videoPlaybackSettings.clear();  // Clear existing playback settings
    
    // Load global settings
    if(layout.contains("settings")) {
        VideoElement::showGradient = layout["settings"]["showGradient"];
        gradientToggle = VideoElement::showGradient;
    }
    
    // Create a map of paths to indices for videos
    map<string, size_t> videoPathToIndex;
    
    // Load video paths and create video players
    if(layout.contains("videoPaths")) {
        for(size_t i = 0; i < layout["videoPaths"].size(); i++) {
            string videoPath = layout["videoPaths"][i];
            videos.emplace_back();
            
            if(!videos.back().load(videoPath)) {
                ofLog() << "Failed to load video: " << videoPath;
                videos.pop_back();
                continue;
            }
            
            videoPathToIndex[videoPath] = videos.size() - 1;
            videos.back().play();
            
            // Load playback settings if available
            if(layout.contains("videoPlaybackSettings") && i < layout["videoPlaybackSettings"].size()) {
                const auto& settings = layout["videoPlaybackSettings"][i];
                APlaybackMode mode = static_cast<APlaybackMode>(settings["mode"].get<int>());
                AOscInputType oscType = static_cast<AOscInputType>(settings["oscType"].get<int>());
                videoPlaybackSettings.push_back(std::make_tuple(mode, oscType));
            } else {
                // Use default settings if not available
                videoPlaybackSettings.push_back(std::make_tuple(APlaybackMode::LOOP, AOscInputType::YAW));
            }
            
            ofLog() << "Successfully loaded video: " << videoPath << " (index " << videos.size()-1 << ")";
        }
    }
    
    // Load image paths and create image objects
    if(layout.contains("imagePaths")) {
        for(const auto& path : layout["imagePaths"]) {
            images.emplace_back();
            if(!images.back().load(path)) {
                ofLog() << "Failed to load image: " << path;
                images.pop_back();
            } else {
                ofLog() << "Successfully loaded image: " << path;
            }
        }
    }
    
    // Load video tiles
    if(layout.contains("videoTiles")) {
        for(const auto& tileData : layout["videoTiles"]) {
            VideoElement tile;
            tile.x = tileData["x"];
            tile.y = tileData["y"];
            tile.offsetX = tileData["offsetX"];
            tile.offsetY = tileData["offsetY"];
            
            ofRectangle region(
                tileData["sourceRegion"]["x"],
                tileData["sourceRegion"]["y"],
                tileData["sourceRegion"]["width"],
                tileData["sourceRegion"]["height"]
            );
            
            tile.setup(tile.x, tile.y);
            
            // Get the path and find its corresponding index
            string path = tileData["path"].get<string>();
            if(videoPathToIndex.find(path) != videoPathToIndex.end()) {
                size_t videoIndex = videoPathToIndex[path];
                tile.setVideoRegion(videoIndex, region);
                tile.setPath(path);
                
                if(tileData.contains("isPrimary")) {
                    tile.setPrimary(tileData["isPrimary"]);
                }
                if(tileData.contains("useColorInput")) {
                    tile.setColorInput(tileData["useColorInput"]);
                }
                if(tileData.contains("colorIndex1") && tileData.contains("colorIndex2")) {
                    tile.setColorIndices(tileData["colorIndex1"], tileData["colorIndex2"]);
                }
                
                tiles.push_back(tile);
            } else {
                ofLog() << "Warning: Could not find video for path: " << path;
            }
        }
    }
    
    // Load image tiles
    if(layout.contains("imageTiles")) {
        for(const auto& tileData : layout["imageTiles"]) {
            ImageElement tile;
            tile.x = tileData["x"];
            tile.y = tileData["y"];
            tile.offsetX = tileData["offsetX"];
            tile.offsetY = tileData["offsetY"];
            
            ofRectangle region(
                tileData["sourceRegion"]["x"],
                tileData["sourceRegion"]["y"],
                tileData["sourceRegion"]["width"],
                tileData["sourceRegion"]["height"]
            );
            
            tile.setup(tile.x, tile.y);
            tile.setImageRegion(tileData["imageIndex"], region);
            
            if(tileData.contains("isPrimary")) {
                tile.setPrimary(tileData["isPrimary"]);
            }
            if(tileData.contains("useColorInput")) {
                tile.setColorInput(tileData["useColorInput"]);
            }
            if(tileData.contains("colorIndex1") && tileData.contains("colorIndex2")) {
                tile.setColorIndices(tileData["colorIndex1"], tileData["colorIndex2"]);
            }
            if(tileData.contains("path")) {
                tile.setPath(tileData["path"]);
            }
            
            imageTiles.push_back(tile);
        }
    }
    
    // Load camera tiles
    if(layout.contains("cameraTiles")) {
        setupCamera();  // Ensure camera is available
        for(const auto& tileData : layout["cameraTiles"]) {
            CameraElement tile;
            tile.x = tileData["x"];
            tile.y = tileData["y"];
            tile.offsetX = tileData["offsetX"];
            tile.offsetY = tileData["offsetY"];
            
            ofRectangle region(
                tileData["sourceRegion"]["x"],
                tileData["sourceRegion"]["y"],
                tileData["sourceRegion"]["width"],
                tileData["sourceRegion"]["height"]
            );
            
            tile.setup(tile.x, tile.y);
            tile.setCameraRegion(tileData["cameraIndex"], region);
            
            if(tileData.contains("isPrimary")) {
                tile.setPrimary(tileData["isPrimary"]);
            }
            if(tileData.contains("useColorInput")) {
                tile.setColorInput(tileData["useColorInput"]);
            }
            if(tileData.contains("colorIndex1") && tileData.contains("colorIndex2")) {
                tile.setColorIndices(tileData["colorIndex1"], tileData["colorIndex2"]);
            }
            
            cameraTiles.push_back(tile);
        }
    }
    
    // Update GUI elements
    updatePrimaryVideoDropdown();
    
    ofLog() << "Layout loaded from: " << path;
    ofLog() << "Loaded " << images.size() << " images";
    ofLog() << "Loaded " << videos.size() << " videos";
}

void ofApp::nextLayout() {
    if(!layoutFiles.empty()) {
        selectedLayout = (selectedLayout + 1) % layoutFiles.size();
        currentLayoutLabel = layoutFiles[selectedLayout];
    }
}

void ofApp::previousLayout() {
    if(!layoutFiles.empty()) {
        selectedLayout = (selectedLayout - 1 + layoutFiles.size()) % layoutFiles.size();
        currentLayoutLabel = layoutFiles[selectedLayout];
    }
}

void ofApp::updateInfoPanel() {
    if(!isEditMode() || selectedTile < 0) return;
    
    // Get the selected tile
    BaseElement* tile = nullptr;
    size_t videoTilesEnd = tiles.size();
    size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
    
    if(selectedTile < videoTilesEnd) {
        tile = &tiles[selectedTile];
    } else if(selectedTile < imageTilesEnd) {
        tile = &imageTiles[selectedTile - videoTilesEnd];
    } else {
        tile = &cameraTiles[selectedTile - imageTilesEnd];
    }
    
    if(tile) {
        // Update tile info
        tileIndexLabel = "Tile Index: " + ofToString(selectedTile);
        tilePosLabel = "Position: " + ofToString(tile->x) + ", " + ofToString(tile->y);
        tileSizeLabel = "Source Region: " + 
            ofToString(tile->sourceRegion.x) + ", " + 
            ofToString(tile->sourceRegion.y) + ", " + 
            ofToString(tile->sourceRegion.width) + ", " + 
            ofToString(tile->sourceRegion.height);
        
        // Remove listeners before updating values
        colorInputToggle.removeListener(this, &ofApp::onColorInputToggled);
        color1Index.removeListener(this, &ofApp::onColor1Changed);
        color2Index.removeListener(this, &ofApp::onColor2Changed);
        
        // Update color input controls to match the selected tile
        colorInputToggle = tile->hasColorInput();
        color1Index = tile->getColorIndex1();
        color2Index = tile->getColorIndex2();
        
        // Re-add listeners after updating values
        colorInputToggle.addListener(this, &ofApp::onColorInputToggled);
        color1Index.addListener(this, &ofApp::onColor1Changed);
        color2Index.addListener(this, &ofApp::onColor2Changed);
    }
}

void ofApp::changeSelectedVideo() {
    if(!isEditMode() || selectedTile >= tiles.size()) return;
    
    ofFileDialogResult result = ofSystemLoadDialog("Select Video File", false, "videos/");
    if(result.bSuccess) {
        string path = result.getPath();
        
        // Create new video player
        videos.emplace_back();
        size_t newVideoIndex = videos.size() - 1;
        
        if(!videos.back().load(path)) {
            ofLog() << "Failed to load video: " << path;
            videos.pop_back();
            return;
        }
        
        videos.back().play();
        
        // Update all tiles that use the same video as the selected tile
        size_t oldVideoIndex = tiles[selectedTile].videoIndex;
        for(auto& tile : tiles) {
            if(tile.videoIndex == oldVideoIndex) {
                tile.videoIndex = newVideoIndex;
                tile.setPath(path);  // Make sure to update the path
            }
        }
        
        // Save changes to current layout
        saveCurrentLayout();
    }
}

void ofApp::saveCurrentLayout() {
    if(layoutFiles.empty() || selectedLayout >= layoutFiles.size()) return;
    
    string path = getLayoutPath(layoutFiles[selectedLayout]);
    ofJson layout;
    
    // Save global settings
    layout["settings"] = {
        {"showGradient", VideoElement::showGradient}
    };
    
    // Save video paths and playback settings
    layout["videoPaths"] = nlohmann::json::array();
    layout["videoPlaybackSettings"] = nlohmann::json::array();
    for(const auto& tile : tiles) {
        string path = tile.getPath();
        // Only add unique paths and their corresponding playback settings
        if(!path.empty() && 
           find(layout["videoPaths"].begin(), layout["videoPaths"].end(), path) == layout["videoPaths"].end()) {
            layout["videoPaths"].push_back(path);
            
            // Save playback settings for this video
            if(tile.videoIndex < videoPlaybackSettings.size()) {
                const auto& settings = videoPlaybackSettings[tile.videoIndex];
                ofJson settingsJson;
                settingsJson["mode"] = static_cast<int>(std::get<0>(settings));
                settingsJson["oscType"] = static_cast<int>(std::get<1>(settings));
                layout["videoPlaybackSettings"].push_back(settingsJson);
            }
        }
    }
    
    // Save image paths
    layout["imagePaths"] = nlohmann::json::array();
    for(const auto& tile : imageTiles) {
        string path = tile.getPath();
        // Only add unique paths
        if(find(layout["imagePaths"].begin(), layout["imagePaths"].end(), path) == layout["imagePaths"].end()) {
            layout["imagePaths"].push_back(path);
        }
    }
    
    // Save video tiles
    layout["videoTiles"] = nlohmann::json::array();
    for(const auto& tile : tiles) {
        ofJson tileData;
        tileData["videoIndex"] = tile.videoIndex;
        tileData["x"] = tile.x;
        tileData["y"] = tile.y;
        tileData["offsetX"] = tile.offsetX;
        tileData["offsetY"] = tile.offsetY;
        tileData["sourceRegion"] = {
            {"x", tile.sourceRegion.x},
            {"y", tile.sourceRegion.y},
            {"width", tile.sourceRegion.width},
            {"height", tile.sourceRegion.height}
        };
        tileData["isPrimary"] = tile.isPrimary();
        tileData["useColorInput"] = tile.hasColorInput();
        tileData["colorIndex1"] = tile.getColorIndex1();
        tileData["colorIndex2"] = tile.getColorIndex2();
        tileData["path"] = tile.getPath();  // Save the path with each tile
        layout["videoTiles"].push_back(tileData);
    }
    
    // Save image tiles
    layout["imageTiles"] = nlohmann::json::array();
    for(const auto& tile : imageTiles) {
        ofJson tileData;
        tileData["imageIndex"] = tile.imageIndex;
        tileData["x"] = tile.x;
        tileData["y"] = tile.y;
        tileData["offsetX"] = tile.offsetX;
        tileData["offsetY"] = tile.offsetY;
        tileData["sourceRegion"] = {
            {"x", tile.sourceRegion.x},
            {"y", tile.sourceRegion.y},
            {"width", tile.sourceRegion.width},
            {"height", tile.sourceRegion.height}
        };
        tileData["isPrimary"] = tile.isPrimary();
        tileData["useColorInput"] = tile.hasColorInput();
        tileData["colorIndex1"] = tile.getColorIndex1();
        tileData["colorIndex2"] = tile.getColorIndex2();
        tileData["path"] = tile.getPath();  // Save the path with each tile
        layout["imageTiles"].push_back(tileData);
    }
    
    // Save camera tiles
    layout["cameraTiles"] = nlohmann::json::array();
    for(const auto& tile : cameraTiles) {
        ofJson tileData;
        tileData["cameraIndex"] = tile.cameraIndex;
        tileData["x"] = tile.x;
        tileData["y"] = tile.y;
        tileData["offsetX"] = tile.offsetX;
        tileData["offsetY"] = tile.offsetY;
        tileData["sourceRegion"] = {
            {"x", tile.sourceRegion.x},
            {"y", tile.sourceRegion.y},
            {"width", tile.sourceRegion.width},
            {"height", tile.sourceRegion.height}
        };
        tileData["isPrimary"] = tile.isPrimary();
        tileData["useColorInput"] = tile.hasColorInput();
        tileData["colorIndex1"] = tile.getColorIndex1();
        tileData["colorIndex2"] = tile.getColorIndex2();
        layout["cameraTiles"].push_back(tileData);
    }
    
    ofSavePrettyJson(path, layout);
    ofLog() << "Layout saved to: " << path;
}

void ofApp::setupOsc() {
    oscReceiver.setup(OSC_PORT);
}

void ofApp::updateOsc() {
    while(oscReceiver.hasWaitingMessages()) {
        ofxOscMessage m;
        oscReceiver.getNextMessage(m);
        
        float value = 0;
        
        if(m.getAddress() == "/yaw") {
            value = m.getArgAsFloat(0);
            ofLog() << "OSC Yaw received: " << value;
            yawValue = value;
        }
        else if(m.getAddress() == "/pitch") {
            value = m.getArgAsFloat(0);
            ofLog() << "OSC Pitch received: " << value;
            pitchValue = value;
        }
        else if(m.getAddress() == "/roll") {
            value = m.getArgAsFloat(0);
            ofLog() << "OSC Roll received: " << value;
            rollValue = value;
        }
    }
}


void ofApp::loadNewVideo() {
    ofFileDialogResult result = ofSystemLoadDialog("Select Video File", false, "videos/");
    if(result.bSuccess) {
        string path = result.getPath();
        
        // Create new video player
        videos.emplace_back();
        size_t newVideoIndex = videos.size() - 1;
        
        if(!videos.back().load(path)) {
            ofLog() << "Failed to load video: " << path;
            videos.pop_back();
            return;
        }
        
        videos.back().play();
        
        // Get video dimensions
        float videoWidth = videos.back().getWidth();
        float videoHeight = videos.back().getHeight();
        
        // Calculate number of tiles needed to cover the video
        int tilesX = ceil(videoWidth / VideoElement::TILE_SIZE);
        int tilesY = ceil(videoHeight / VideoElement::TILE_SIZE);
        
        // Create tiles for the video
        for(int y = 0; y < tilesY; y++) {
            for(int x = 0; x < tilesX; x++) {
                VideoElement tile;
                
                // Calculate position for the tile
                float posX = x * VideoElement::TILE_SIZE;
                float posY = y * VideoElement::TILE_SIZE;
                
                // Calculate source region for this tile
                ofRectangle region;
                region.x = x * VideoElement::TILE_SIZE;
                region.y = y * VideoElement::TILE_SIZE;
                region.width = std::min(static_cast<float>(VideoElement::TILE_SIZE), 
                                      videoWidth - region.x);
                region.height = std::min(static_cast<float>(VideoElement::TILE_SIZE), 
                                       videoHeight - region.y);
                
                tile.setup(posX, posY);
                tile.setVideoRegion(newVideoIndex, region);
                tile.setPath(path);  // Make sure to set the path
                tiles.push_back(tile);
            }
        }
        
        // Save the current layout
        saveCurrentLayout();
        
        ofLog() << "Added new video with " << (tilesX * tilesY) << " tiles";
    }
}

void ofApp::onGradientToggled(bool& value) {
    VideoElement::showGradient = value;
}

void ofApp::updatePrimaryVideoDropdown() {
    // Get unique video paths
    vector<string> uniquePaths = getUniqueVideoPaths();
    
    // Update dropdown max value
    primaryVideoIndex.setMax(uniquePaths.size() - 1);
    
    // Update label with current primary video info
    int currentPrimary = -1;
    string primaryPath = "None";
    
    for(size_t i = 0; i < tiles.size(); i++) {
        if(tiles[i].isPrimary()) {
            currentPrimary = tiles[i].videoIndex;
            if(currentPrimary < videos.size()) {
                primaryPath = ofFilePath::getFileName(videos[currentPrimary].getMoviePath());
            }
            break;
        }
    }
    
    primaryVideoLabel = "Primary: " + primaryPath;
    primaryVideoIndex = currentPrimary;
}

void ofApp::onPrimaryVideoChanged(int& index) {
    // Clear existing primary status
    for(auto& tile : tiles) {
        tile.setPrimary(false);
    }
    
    // Set new primary video
    if(index >= 0) {
        for(auto& tile : tiles) {
            if(tile.videoIndex == index) {
                tile.setPrimary(true);
            }
        }
    }
    
    updatePrimaryVideoDropdown();
    needsSwatchUpdate = true;  // Request swatch update when primary changes
    saveCurrentLayout();
}

vector<string> ofApp::getUniqueVideoPaths() const {
    vector<string> paths;
    for(const auto& tile : tiles) {
        string path = tile.getPath();
        // Only add unique paths
        if(!path.empty() && find(paths.begin(), paths.end(), path) == paths.end()) {
            paths.push_back(path);
        }
    }
    return paths;
}

void ofApp::updateColorSwatchesFromPrimary() {
    // Find primary video
    int primaryVideoIndex = -1;
    for(const auto& tile : tiles) {
        if(tile.isPrimary()) {
            primaryVideoIndex = tile.videoIndex;
            break;
        }
    }
    
    if(primaryVideoIndex >= 0 && primaryVideoIndex < videos.size()) {
        const auto& video = videos[primaryVideoIndex];
        if(!video.isLoaded()) return;
        
        // Get current frame
        ofPixels pixels = video.getPixels();
        
        // Only reallocate if necessary
        float aspect = (float)video.getHeight() / video.getWidth();
        int processHeight = PROCESS_WIDTH * aspect;
        
        if(!isCvImageAllocated || 
           cvImage.getWidth() != PROCESS_WIDTH || 
           cvImage.getHeight() != processHeight) {
            cvImage.clear();
            cvImage.allocate(PROCESS_WIDTH, processHeight);
            isCvImageAllocated = true;
        }
        
        cvImage.setFromPixels(pixels);
        cvImage.resize(PROCESS_WIDTH, processHeight);
        ofPixels smallPixels = cvImage.getPixels();
        
        // Extract colors as HSV
        vector<ofVec3f> colors;
        for(int y = 0; y < processHeight; y++) {
            for(int x = 0; x < PROCESS_WIDTH; x++) {
                ofColor c = smallPixels.getColor(x, y);
                float hue, sat, val;
                c.getHsb(hue, sat, val);
                colors.push_back(ofVec3f(hue, sat, val));
            }
        }
        
        // K-means clustering
        vector<ofVec3f> centroids = colors;
        random_shuffle(centroids.begin(), centroids.end());
        centroids.resize(NUM_SWATCHES);
        
        vector<vector<ofVec3f>> clusters(NUM_SWATCHES);
        for(const auto& color : colors) {
            float minDist = FLT_MAX;
            int closestCentroid = 0;
            
            for(int i = 0; i < NUM_SWATCHES; i++) {
                float dist = (color - centroids[i]).length();
                if(dist < minDist) {
                    minDist = dist;
                    closestCentroid = i;
                }
            }
            
            clusters[closestCentroid].push_back(color);
        }
        
        // Calculate new colors
        vector<ofColor> newColors(NUM_SWATCHES);
        for(int i = 0; i < NUM_SWATCHES; i++) {
            if(!clusters[i].empty()) {
                ofVec3f sum(0, 0, 0);
                for(const auto& color : clusters[i]) {
                    sum += color;
                }
                ofVec3f centroid = sum / clusters[i].size();
                
                newColors[i].setHsb(
                    centroid.x,
                    centroid.y,
                    centroid.z
                );
            }
        }
        
        // Sort colors by brightness (value component)
        sort(newColors.begin(), newColors.end(), [](const ofColor& a, const ofColor& b) {
            float ha, sa, va, hb, sb, vb;
            a.getHsb(ha, sa, va);
            b.getHsb(hb, sb, vb);
            return va > vb; // Sort descending (brightest first)
        });
        
        // Update color swatches with sorted colors
        colorSwatches = newColors;
    }
}

// Add this helper function to calculate color distance
float ofApp::colorDistance(const ofColor& c1, const ofColor& c2) {
    float h1, s1, b1, h2, s2, b2;
    c1.getHsb(h1, s1, b1);
    c2.getHsb(h2, s2, b2);
    
    // Handle hue wrap-around
    float hueDiff = abs(h1 - h2);
    if(hueDiff > 180) {
        hueDiff = 360 - hueDiff;
    }
    
    // Weight hue differences less than saturation and brightness
    return sqrt(pow(hueDiff * 0.5f, 2) + pow(s1 - s2, 2) + pow(b1 - b2, 2));
}

void ofApp::drawColorSwatches() {
    if(colorSwatches.empty()) return;
    
    float x = swatchPanel.getPosition().x;
    float y = swatchPanel.getPosition().y + 20;  // Leave room for panel header
    float swatchSize = 30;
    float padding = 5;
    
    ofPushStyle();
    
    // Draw panel background
    ofSetColor(swatchPanel.getBackgroundColor());
    ofDrawRectangle(x, y - 20, swatchSize * NUM_SWATCHES + padding * (NUM_SWATCHES + 1), 
                    swatchSize + padding * 2 + 20);
    
    // Draw swatches
    for(int i = 0; i < colorSwatches.size(); i++) {
        float swatchX = x + padding + (swatchSize + padding) * i;
        
        // Draw swatch
        ofSetColor(colorSwatches[i]);
        ofDrawRectangle(swatchX, y, swatchSize, swatchSize);
        
        // Draw border
        ofNoFill();
        ofSetColor(128);
        ofDrawRectangle(swatchX, y, swatchSize, swatchSize);
        ofFill();
    }
    
    ofPopStyle();
}

void ofApp::onColorInputToggled(bool& value) {
    if(!isEditMode() || selectedTile < 0) return;
    
    // Get the selected tile
    BaseElement* tile = nullptr;
    size_t videoTilesEnd = tiles.size();
    size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
    
    if(selectedTile < videoTilesEnd) {
        tile = &tiles[selectedTile];
    } else if(selectedTile < imageTilesEnd) {
        tile = &imageTiles[selectedTile - videoTilesEnd];
    } else {
        tile = &cameraTiles[selectedTile - imageTilesEnd];
    }
    
    if(tile) {
        // Set color input for this specific tile
        tile->setColorInput(value);
        
        // Update the toggle to reflect the current state
        colorInputToggle = value;
        
        // Save the current layout
        saveCurrentLayout();
    }
}

void ofApp::onColor1Changed(int& index) {
    if(!isEditMode() || selectedTile < 0) return;
    
    // Get the selected tile
    BaseElement* tile = nullptr;
    size_t videoTilesEnd = tiles.size();
    size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
    
    if(selectedTile < videoTilesEnd) {
        tile = &tiles[selectedTile];
    } else if(selectedTile < imageTilesEnd) {
        tile = &imageTiles[selectedTile - videoTilesEnd];
    } else {
        tile = &cameraTiles[selectedTile - imageTilesEnd];
    }
    
    if(tile) {
        // Set color index for this specific tile
        tile->setColorIndices(index, tile->getColorIndex2());
        saveCurrentLayout();
    }
}

void ofApp::onColor2Changed(int& index) {
    if(!isEditMode() || selectedTile < 0) return;
    
    // Get the selected tile
    BaseElement* tile = nullptr;
    size_t videoTilesEnd = tiles.size();
    size_t imageTilesEnd = videoTilesEnd + imageTiles.size();
    
    if(selectedTile < videoTilesEnd) {
        tile = &tiles[selectedTile];
    } else if(selectedTile < imageTilesEnd) {
        tile = &imageTiles[selectedTile - videoTilesEnd];
    } else {
        tile = &cameraTiles[selectedTile - imageTilesEnd];
    }
    
    if(tile) {
        // Set color index for this specific tile
        tile->setColorIndices(tile->getColorIndex1(), index);
        saveCurrentLayout();
    }
}

void ofApp::loadNewImage() {
    ofFileDialogResult result = ofSystemLoadDialog("Select Image File", false, "images/");
    if(result.bSuccess) {
        string path = result.getPath();
        
        // Create new image
        images.emplace_back();
        size_t newImageIndex = images.size() - 1;
        
        if(!images.back().load(path)) {
            ofLog() << "Failed to load image: " << path;
            images.pop_back();
            return;
        }
        
        // Get image dimensions
        float imageWidth = images.back().getWidth();
        float imageHeight = images.back().getHeight();
        
        // Calculate number of tiles needed to cover the image
        int tilesX = ceil(imageWidth / ImageElement::TILE_SIZE);
        int tilesY = ceil(imageHeight / ImageElement::TILE_SIZE);
        
        // Create tiles for the image
        for(int y = 0; y < tilesY; y++) {
            for(int x = 0; x < tilesX; x++) {
                ImageElement tile;
                
                // Calculate position for the tile
                float posX = x * ImageElement::TILE_SIZE;
                float posY = y * ImageElement::TILE_SIZE;
                
                // Calculate source region for this tile
                ofRectangle region;
                region.x = x * ImageElement::TILE_SIZE;
                region.y = y * ImageElement::TILE_SIZE;
                region.width = std::min(static_cast<float>(ImageElement::TILE_SIZE), 
                                      imageWidth - region.x);
                region.height = std::min(static_cast<float>(ImageElement::TILE_SIZE), 
                                       imageHeight - region.y);
                
                tile.setup(posX, posY);
                tile.setImageRegion(newImageIndex, region);
                tile.setPath(path);  // Store the path for later use
                imageTiles.push_back(tile);
            }
        }
        
        // Save the current layout
        saveCurrentLayout();
        
        ofLog() << "Added new image with " << (tilesX * tilesY) << " tiles";
    }
}

void ofApp::setupCamera() {
    // Setup default camera if none exists
    if(cameras.empty()) {
        cameras.emplace_back();
        cameras.back().setup(640, 480);
    }
}

void ofApp::addCameraTile() {
    setupCamera();  // Ensure we have a camera
    
    if(!cameras.empty() && cameras[0].isInitialized()) {
        // Set desired camera dimensions
        const int CAMERA_WIDTH = 512;
        const int CAMERA_HEIGHT = 512;
        cameras[0].setDesiredFrameRate(30);
        cameras[0].setup(CAMERA_WIDTH, CAMERA_HEIGHT);
        
        // Calculate number of tiles needed
        int tilesX = ceil(float(CAMERA_WIDTH) / CameraElement::TILE_SIZE);
        int tilesY = ceil(float(CAMERA_HEIGHT) / CameraElement::TILE_SIZE);
        
        // Calculate starting position for this set of tiles
        float startX = 10;
        float startY = 10;
        
        // Create tiles
        for(int y = 0; y < tilesY; y++) {
            for(int x = 0; x < tilesX; x++) {
                CameraElement tile;
                
                // Position tile
                float tileX = startX + x * CameraElement::TILE_SIZE;
                float tileY = startY + y * CameraElement::TILE_SIZE;
                tile.setup(tileX, tileY);
                
                // Calculate source region for this tile
                ofRectangle region(
                    x * CameraElement::TILE_SIZE,
                    y * CameraElement::TILE_SIZE,
                    min(CameraElement::TILE_SIZE, CAMERA_WIDTH - x * CameraElement::TILE_SIZE),
                    min(CameraElement::TILE_SIZE, CAMERA_HEIGHT - y * CameraElement::TILE_SIZE)
                );
                
                tile.setCameraRegion(0, region);  // Use first camera
                cameraTiles.push_back(tile);
            }
        }
        
        // Save the current layout
        saveCurrentLayout();
        
        ofLog() << "Added new camera with " << (tilesX * tilesY) << " tiles";
    } else {
        ofLog() << "Failed to add camera tile - no camera available";
    }
}

template<typename T>
void ofApp::loadTileData(T& tile, const ofJson& tileData, size_t newIndex) {
    // Load position and offset
    tile.x = tileData["x"];
    tile.y = tileData["y"];
    tile.offsetX = tileData["offsetX"];
    tile.offsetY = tileData["offsetY"];
    
    // Load source region
    ofRectangle region(
        tileData["sourceRegion"]["x"],
        tileData["sourceRegion"]["y"],
        tileData["sourceRegion"]["width"],
        tileData["sourceRegion"]["height"]
    );
    
    // Setup tile and region
    tile.setup(tile.x, tile.y);
    
    // Set region based on tile type using template specialization
    setRegionForTile(tile, newIndex, region, typename std::is_same<T, VideoElement>::type());
    setRegionForTile(tile, newIndex, region, typename std::is_same<T, ImageElement>::type());
    setRegionForTile(tile, newIndex, region, typename std::is_same<T, CameraElement>::type());
    
    // Load optional properties
    if(tileData.contains("isPrimary")) {
        tile.setPrimary(tileData["isPrimary"]);
    }
    if(tileData.contains("useColorInput")) {
        tile.setColorInput(tileData["useColorInput"]);
    }
    if(tileData.contains("colorIndex1") && tileData.contains("colorIndex2")) {
        tile.setColorIndices(tileData["colorIndex1"], tileData["colorIndex2"]);
    }
}

// Helper functions for type-specific region setting
template<typename T>
void ofApp::setRegionForTile(T& tile, size_t index, const ofRectangle& region, std::true_type) {
    // This overload is called when the type matches
    setRegionForSpecificTile(tile, index, region);
}

template<typename T>
void ofApp::setRegionForTile(T& tile, size_t index, const ofRectangle& region, std::false_type) {
    // This overload is called when the type doesn't match - do nothing
}

// Type-specific implementations
void ofApp::setRegionForSpecificTile(VideoElement& tile, size_t index, const ofRectangle& region) {
    tile.setVideoRegion(index, region);
}

void ofApp::setRegionForSpecificTile(ImageElement& tile, size_t index, const ofRectangle& region) {
    tile.setImageRegion(index, region);
}

void ofApp::setRegionForSpecificTile(CameraElement& tile, size_t index, const ofRectangle& region) {
    tile.setCameraRegion(index, region);
}

// Explicit template instantiations
template void ofApp::loadTileData<VideoElement>(VideoElement& tile, const ofJson& tileData, size_t newIndex);
template void ofApp::loadTileData<ImageElement>(ImageElement& tile, const ofJson& tileData, size_t newIndex);
template void ofApp::loadTileData<CameraElement>(CameraElement& tile, const ofJson& tileData, size_t newIndex);

void ofApp::createNewLayout() {
    // Clear all elements
    tiles.clear();
    imageTiles.clear();
    cameraTiles.clear();
    videos.clear();
    images.clear();
    cameras.clear();
    
    // Generate new layout name
    string newLayoutName = generateLayoutName();
    
    // Create empty layout file
    ofJson layout;
    layout["settings"] = {
        {"showGradient", VideoElement::showGradient}
    };
    layout["videoPaths"] = nlohmann::json::array();
    layout["imagePaths"] = nlohmann::json::array();
    layout["videoTiles"] = nlohmann::json::array();
    layout["imageTiles"] = nlohmann::json::array();
    layout["cameraTiles"] = nlohmann::json::array();
    
    // Create layouts directory if it doesn't exist
    ofDirectory dir("layouts");
    if(!dir.exists()) {
        dir.create();
    }
    
    // Save empty layout
    string path = getLayoutPath(newLayoutName);
    ofSavePrettyJson(path, layout);
    ofLog() << "Created new empty layout: " << path;
    
    // Refresh layout list and select new layout
    refreshLayoutList();
    
    // Select the newly created layout
    for(size_t i = 0; i < layoutFiles.size(); i++) {
        if(layoutFiles[i] == newLayoutName) {
            selectedLayout = i;
            currentLayoutLabel = newLayoutName;
            break;
        }
    }
}

void ofApp::setVideoPlaybackMode(size_t videoIndex, APlaybackMode mode, AOscInputType oscType) {
    if(videoIndex < videoPlaybackSettings.size()) {
        videoPlaybackSettings[videoIndex] = std::make_tuple(mode, oscType);
        saveCurrentLayout();  // Save changes to current layout
    }
}

void ofApp::setupVideoPreviewPanel() {
    videoPreviewPanel.setup("Video Preview");
    videoPreviewPanel.add(isOscMode);
    videoPreviewPanel.add(oscInputType);
    
    // Add listeners
    isOscMode.addListener(this, &ofApp::onOscModeChanged);
    oscInputType.addListener(this, &ofApp::onOscInputChanged);
    
    // Position panel
    videoPreviewPanel.setPosition(
        infoPanel.getPosition().x + infoPanel.getWidth() + 10, 
        infoPanel.getPosition().y
    );
    
    /* Comment out preview rectangle setup
    previewRect.set(
        videoPreviewPanel.getPosition().x,
        videoPreviewPanel.getPosition().y + videoPreviewPanel.getHeight() + 10,
        PREVIEW_WIDTH,
        PREVIEW_HEIGHT
    );
    */
}

void ofApp::updateVideoPreviewPanel() {
    if(!isEditMode() || selectedTile < 0 || selectedTile >= tiles.size()) {
        showVideoPreview = false;
        return;
    }
    
    showVideoPreview = true;
    const auto& tile = tiles[selectedTile];
    
    if(tile.videoIndex < videoPlaybackSettings.size()) {
        // Update controls to match current settings
        const auto& settings = videoPlaybackSettings[tile.videoIndex];
        
        // Remove listeners temporarily to avoid triggering callbacks
        isOscMode.removeListener(this, &ofApp::onOscModeChanged);
        oscInputType.removeListener(this, &ofApp::onOscInputChanged);
        
        isOscMode = (std::get<0>(settings) == APlaybackMode::OSC_PLAYBACK);
        oscInputType = static_cast<int>(std::get<1>(settings));
        
        // Re-add listeners
        isOscMode.addListener(this, &ofApp::onOscModeChanged);
        oscInputType.addListener(this, &ofApp::onOscInputChanged);
    }
}

void ofApp::drawVideoPreview() {
    if(!isEditMode() || selectedTile < 0 || selectedTile >= tiles.size()) return;
    
    const auto& tile = tiles[selectedTile];
    if(tile.videoIndex < videos.size()) {
        ofVideoPlayer& video = videos[tile.videoIndex];
        if(video.isLoaded()) {
            // Draw video preview
            ofPushStyle();
            ofSetColor(255);
            video.draw(previewRect);
            
            // Draw border
            ofNoFill();
            ofSetColor(128);
            ofDrawRectangle(previewRect);
            ofPopStyle();
        }
    }
}

void ofApp::onOscModeChanged(bool& value) {
    if(!isEditMode() || selectedTile < 0 || selectedTile >= tiles.size()) return;
    
    const auto& tile = tiles[selectedTile];
    if(tile.videoIndex < videoPlaybackSettings.size()) {
        APlaybackMode mode = value ? APlaybackMode::OSC_PLAYBACK : APlaybackMode::LOOP;
        AOscInputType currentOscType = std::get<1>(videoPlaybackSettings[tile.videoIndex]);
        setVideoPlaybackMode(tile.videoIndex, mode, currentOscType);
    }
}

void ofApp::onOscInputChanged(int& value) {
    if(!isEditMode() || selectedTile < 0 || selectedTile >= tiles.size()) return;
    
    const auto& tile = tiles[selectedTile];
    if(tile.videoIndex < videoPlaybackSettings.size()) {
        APlaybackMode currentMode = std::get<0>(videoPlaybackSettings[tile.videoIndex]);
        AOscInputType oscType = static_cast<AOscInputType>(value);
        setVideoPlaybackMode(tile.videoIndex, currentMode, oscType);
    }
}

void ofApp::alignTilesToGrid() {
    // Calculate grid cell size (tile size + spacing)
    const int cellSize = VideoElement::TILE_SIZE + GRID_SPACING;
    
    // Calculate grid offset to center it
    float gridWidth = GRID_COLS * cellSize - GRID_SPACING;  // Subtract last spacing
    float gridHeight = GRID_ROWS * cellSize - GRID_SPACING; // Subtract last spacing
    float startX = (ofGetWidth() - gridWidth) / 2;
    float startY = (ofGetHeight() - gridHeight) / 2;
    
    // Function to find nearest grid position
    auto snapToGrid = [&](float x, float y) -> ofPoint {
        // Convert to grid-relative coordinates
        float relX = x - startX;
        float relY = y - startY;
        
        // Find nearest grid cell
        int col = round(relX / cellSize);
        int row = round(relY / cellSize);
        
        // Clamp to grid bounds
        col = ofClamp(col, 0, GRID_COLS - 1);
        row = ofClamp(row, 0, GRID_ROWS - 1);
        
        // Convert back to screen coordinates
        return ofPoint(
            startX + col * cellSize,
            startY + row * cellSize
        );
    };
    
    // Record moves for undo
    vector<int> movedTiles;
    
    // Align video tiles
    for(size_t i = 0; i < tiles.size(); i++) {
        ofPoint newPos = snapToGrid(tiles[i].x, tiles[i].y);
        tiles[i].x = newPos.x;
        tiles[i].y = newPos.y;
        movedTiles.push_back(i);
    }
    
    // Align image tiles
    for(size_t i = 0; i < imageTiles.size(); i++) {
        ofPoint newPos = snapToGrid(imageTiles[i].x, imageTiles[i].y);
        imageTiles[i].x = newPos.x;
        imageTiles[i].y = newPos.y;
        movedTiles.push_back(i + tiles.size());
    }
    
    // Align camera tiles
    for(size_t i = 0; i < cameraTiles.size(); i++) {
        ofPoint newPos = snapToGrid(cameraTiles[i].x, cameraTiles[i].y);
        cameraTiles[i].x = newPos.x;
        cameraTiles[i].y = newPos.y;
        movedTiles.push_back(i + tiles.size() + imageTiles.size());
    }
    
    // Record the move for undo
    if(!movedTiles.empty()) {
        recordTileMove(movedTiles, true);
        saveCurrentLayout();
    }
}
