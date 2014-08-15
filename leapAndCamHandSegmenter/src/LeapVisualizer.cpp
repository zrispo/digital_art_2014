//
//  LeapVisualizer.cpp
//  leapAndCamRecorder2
//
//  Created by chris on 28/07/14.
//
//

#include "LeapVisualizer.h"

//-----------------------------------------------------------------
void LeapVisualizer::setup(){
    
    bDrawSimple				= false;
    bDrawGrid				= true;
	bDrawDiagnosticColors	= true;
	bProjectorSet			= false;
	
	bEnableVoronoiRendering		= false;
	bUseVoronoiFbo				= false;
	bActuallyDisplayVoronoiFbo	= false;
	bWorkAtHalfScale			= false;
	
    diagnosticFingerScaling = 1.2;
}

//-----------------------------------------------------------------
void LeapVisualizer::enableVoronoiRendering (int imgW, int imgH, bool bHalved){
	
	bEnableVoronoiRendering		= true;
	bUseVoronoiFbo				= true;
	bActuallyDisplayVoronoiFbo	= true;
	bWorkAtHalfScale			= bHalved;
	
	voronoiFbo.allocate(imgW,imgH, GL_RGB);
	
	NB = new nbody (imgW, imgH);
	initPointsToVoronoi();
	
	nbody_w = NB->ResX;
	nbody_h = NB->ResY;
}

//--------------------------------------------------------------
void LeapVisualizer::initPointsToVoronoi(){
	if (bEnableVoronoiRendering){
		// Initialize 5 (fixed-length) bodies with 5 (bogus) points each.
		
		float tmp_xpts[5];
		float tmp_ypts[5];
		float px, py;
		
		int nFingers = 5;
		int nJointPointsPerFinger = 5;
		
		for (int i=0; i<nFingers; i++){ // NUM_NBODIES = 5 fingers, in nbody.h
			PolygonObject *Poly = NB->Polys[i];
			
			float ry = 0.2f + 0.6*ofRandom(0,1);
			for (int j=0; j<nJointPointsPerFinger; j++){
				px = (float)(i+1) / (float)(nFingers+1) + ofRandom(0,1)*0.015f;
				px = (px * 2.0) - 1.0;
				
				py = ry + 0.20*((float)j/(float)nJointPointsPerFinger - 0.5f);
				py = (py * 2.0) - 1.0;
				
				tmp_xpts[j] = px;
				tmp_ypts[j] = py;
			}
			Poly->SetBoundary (tmp_xpts, tmp_ypts, nJointPointsPerFinger);
		}
	}
}


//--------------------------------------------------------------
void LeapVisualizer::updateVoronoi(){
	if (bEnableVoronoiRendering){
		NB->myIdle();
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawVoronoi(){
	if (bEnableVoronoiRendering){
		
		if (bUseVoronoiFbo){
			voronoiFbo.begin();
			ofClear(0,0,0,0);
		}
		
		// Render the actual voronoi shapes
		NB->myDisplay();

		// For diagnostic purposes, draw the skeletons
		bool bDrawVoronoiSkeletons = false;
		if (bDrawVoronoiSkeletons){
			ofPushStyle();
			ofSetColor(255,0,0);
			ofNoFill();
			ofSetLineWidth(3.0);
			for (int i=0; i<NUM_NBODIES; i++){ // 5, in nbody.h
				PolygonObject *Poly = NB->Polys[i];
				float ox = Poly->OrigX;
				float oy = Poly->OrigY;
				
				ofPolyline polyline;
				for (int j=0; j<MAX_NBODY_POLYLINE_LENGTH; j++){
					float px =  (nbody_w/2) +  (nbody_w/2) * (ox + Poly->oPts[j*2  ]);
					float py =  (nbody_h/2) +  (nbody_h/2) * (oy + Poly->oPts[j*2+1]);
					polyline.addVertex(px,py);
				}
				polyline.draw();
			}
			ofSetLineWidth(1.0);
			ofFill();
			ofPopStyle();
		}

		if (bUseVoronoiFbo){
			voronoiFbo.end();
			
			// Actually draw the FBO.
			// Note, it is correctively flipped upside-down.
			if (bActuallyDisplayVoronoiFbo){
				ofSetColor(255);
				voronoiFbo.draw(0,nbody_h, nbody_w,0-nbody_h);
			}
		}
		
	}
}


//--------------------------------------------------------------
void LeapVisualizer::feedBogusPointsToVoronoi(){
	if (bEnableVoronoiRendering){
	
		for (int i=0; i<NUM_NBODIES; i++){
			PolygonObject *Poly = NB->Polys[i];
			
			Poly->OrigX += ofRandom(-1,1)*0.0002;
			Poly->OrigY += ofRandom(-1,1)*0.0002;
			
			for (int j=0; j<MAX_NBODY_POLYLINE_LENGTH; j++){
				Poly->oPts[j*2]    += ofRandom(-1,1)*0.0004;
				Poly->oPts[j*2+1]  += ofRandom(-1,1)*0.0004;
			}
			NB->Polys[i]->outsideUpdate();
		}
	}
}







//-----------------------------------------------------------------
void LeapVisualizer::loadXmlFile(string fileName){
    XML.clear();
    XML.loadFile(fileName);
}


ofPoint LeapVisualizer::getIndexFingertipFromXML(int whichFrame){
    
    // assumes only one hand
    
    ofPoint fingerTip = ofPoint(-1,-1,-1);
    
    int nFrameTags = XML.getNumTags("FRAME");
	if ((whichFrame >= 0) && (whichFrame < nFrameTags)){
		
		//we push into the which'h FRAME tag; this temporarily treats the tag as the document root.
		XML.pushTag("FRAME", whichFrame);
		
		int nHandTags = XML.getNumTags("H");
		if (nHandTags > 0){
			XML.pushTag("H",0);
            int nFingers = XML.getNumTags("F");
            for( int i = 0; i < nFingers; i++){
                XML.pushTag("F",i);
                int fingerType = XML.getValue("TYPE",0);
                if(fingerType ==  Finger::TYPE_INDEX){
                    fingerTip.x = XML.getValue("T:X", 0);
                    fingerTip.y = XML.getValue("T:Y", 0);
                    fingerTip.z = XML.getValue("T:Z", 0);
                    XML.popTag();
                    break;
                }
                XML.popTag();
            }
            XML.popTag();
		}
        XML.popTag();

	}
    
    return fingerTip;
}

ofPoint LeapVisualizer::getIndexFingertip(ofxLeapMotion & leap){
    
    vector <Hand> hands = leap.getLeapHands();
    Hand hand;
    
	if (hands.size() > 0){
		for (int h=0; h<hands.size(); h++){
			hand = hands[h];
			if (hand.isValid()) break;
		}
	}
	
	// For each finger in the Hand,
	FingerList fingers = hand.fingers();
	for (int f=0; f<fingers.count(); f++)
    {
		// Get the current finger, and it's type (index, thumb, etc.);
		const Finger & finger = fingers[f];
		Finger::Type fingerType = finger.type();
		if (finger.isValid() && fingerType == Finger::TYPE_INDEX)
        {
            return ofPoint(finger.tipPosition().x, finger.tipPosition().y, finger.tipPosition().z);
		}
	}
    cout << "FINGER NOT FOUND" << endl;
    return ofPoint(-1.0f,-1.0f,-1.0f);
}


//--------------------------------------------------------------
void LeapVisualizer::setProjector(ofxRay::Projector P){
	screenProjector = P;
	bProjectorSet = true;
}
//--------------------------------------------------------------
ofVec3f LeapVisualizer::getColorDiagnostically (Finger::Type fingerType, Bone::Type boneType,
												ofPoint bonePt0, ofPoint bonePt1){
	
	ofVec3f outputColor;
	outputColor.set(0,0,0);
	
	if (bProjectorSet){
		
		ofVec3f bone0Vec3f = ofVec3f(bonePt0.x, bonePt0.y, bonePt0.z);
		ofVec3f bone1Vec3f = ofVec3f(bonePt1.x, bonePt1.y, bonePt1.z);
		
		ofVec3f screen0Vec3f = screenProjector.getScreenCoordinateOfWorldPosition (bone0Vec3f);
		ofVec3f screen1Vec3f = screenProjector.getScreenCoordinateOfWorldPosition (bone1Vec3f);
		
		float dx = screen1Vec3f.x - screen0Vec3f.x;
		float dy = screen1Vec3f.y - screen0Vec3f.y;
		float dh = sqrtf(dx*dx + dy*dy);
		if (dh > 0){
			
			// Red and green encode orientation; blue encodes ID
			float angle		= atan2f(dy, dx);
			float dangle	= DEG_TO_RAD * (-45.0);
			float boneRed   = 255.0 * abs(cos(angle + dangle));
			float boneGreen = 255.0 * abs(sin(angle + dangle));
			
			//float boneRed   = ofMap( cos(angle + dangle), -1,1, 0,255);
			//float boneGreen = ofMap( sin(angle + dangle), -1,1, 0,255);
			
			float boneBlue  = 0;
			switch (fingerType){
				case Finger::TYPE_THUMB:	boneBlue = (32.0 * ID_THUMB);	break;
				case Finger::TYPE_INDEX:	boneBlue = (32.0 * ID_INDEX);	break;
				case Finger::TYPE_MIDDLE:	boneBlue = (32.0 * ID_MIDDLE);	break;
				case Finger::TYPE_RING:		boneBlue = (32.0 * ID_RING);	break;
				case Finger::TYPE_PINKY:	boneBlue = (32.0 * ID_PINKY);	break;
			}
			outputColor.set(boneRed, boneGreen, boneBlue);
			
			// For the bones inside the palm, set the color to gray.
			bool bSetInternalBonesToGray = false;
			if (bSetInternalBonesToGray){
				if ( (boneType == Bone::TYPE_METACARPAL) ||
					((boneType == Bone::TYPE_PROXIMAL)   && (fingerType == Finger::TYPE_THUMB))) {
					outputColor.set(ID_PALM*32, ID_PALM*32, ID_PALM*32);
				}
			}
			
		} else {
			// printf("Degenerate bone\n");
			outputColor.set(0,0,0);
		}
		
	} else {
		outputColor.set(0,0,0);
	}
	
	return outputColor;
}

//--------------------------------------------------------------
void LeapVisualizer::setColorByFinger (Finger::Type fingerType, Bone::Type boneType){
    
	// Set the current color, according to the type of this finger.
	// Thumb is red, Index is green, etc.
	
	bool bUseGrayscale = false;
	if (bUseGrayscale){
		switch (fingerType){
			case Finger::TYPE_THUMB:
				ofSetColor(ID_THUMB*32);
				break;
			case Finger::TYPE_INDEX:
				ofSetColor(ID_INDEX*32);
				break;
			case Finger::TYPE_MIDDLE:
				ofSetColor(ID_MIDDLE*32);
				break;
			case Finger::TYPE_RING:
				ofSetColor(ID_RING*32);
				break;
			case Finger::TYPE_PINKY:
				ofSetColor(ID_PINKY*32);
				break;
			default:
				ofSetColor(ID_PALM*32);
				break;
		}
		
	} else {
		
		switch (fingerType){
			case Finger::TYPE_THUMB:
				ofSetColor(ofColor::red);
				break;
			case Finger::TYPE_INDEX:
				ofSetColor(ofColor::green);
				break;
			case Finger::TYPE_MIDDLE:
				ofSetColor(ofColor::blue);
				break;
			case Finger::TYPE_RING:
				ofSetColor(ofColor::yellow);
				break;
			case Finger::TYPE_PINKY:
				ofSetColor(ofColor::cyan);
				break;
			default:
				ofSetColor(ofColor::gray);
				break;
		}
	}
	
	// For the bones inside the palm, set the color to gray.
	bool bSetInternalBonesToGray = true;
	if (bSetInternalBonesToGray){
		if ( (boneType == Bone::TYPE_METACARPAL) ||
			((boneType == Bone::TYPE_PROXIMAL)   && (fingerType == Finger::TYPE_THUMB))) {
			
			if (bUseGrayscale){
				ofSetColor(ID_PALM*32);
			} else {
				ofSetColor(ofColor::gray);
			}
		}
	}
}



//--------------------------------------------------------------
void LeapVisualizer::drawOrientedCylinder (ofPoint pt0, ofPoint pt1, float radius){
	
	// Draw a cylinder between two points, properly oriented in space.
	float dx = pt1.x - pt0.x;
	float dy = pt1.y - pt0.y;
	float dz = pt1.z - pt0.z;
	float dh = sqrt(dx*dx + dy*dy + dz*dz);
	
	ofPushMatrix();
    ofTranslate( (pt0.x+pt1.x)/2, (pt0.y+pt1.y)/2, (pt0.z+pt1.z)/2 );
    
    ofQuaternion q;
    q.makeRotate (ofPoint(0, -1, 0), ofPoint(dx,dy,dz) );
    ofMatrix4x4 m;
    q.get(m);
    glMultMatrixf(m.getPtr());
    
    ofDrawCylinder (radius, dh);
	ofPopMatrix();
}

void LeapVisualizer::drawGrid(){
	
	// Draw a grid plane.
	if (bDrawGrid){
		ofPushMatrix();
		ofEnableSmoothing();
		ofRotate(90, 0, 0, 1);
		ofSetColor(160,160,160, 200);
		ofDrawGridPlane(200, 10, false);
		ofPopMatrix();
		ofDisableSmoothing();
	}
}


//--------------------------------------------------------------
void LeapVisualizer::drawVoronoiFrame (ofxLeapMotion & leap){
	// Draw the voronoi extensions of the fingers,
	// using the real Leap controller (not from XML)
	
	if (bEnableVoronoiRendering && bProjectorSet){
		ofDisableSmoothing();
		
		vector <Hand> hands = leap.getLeapHands();
		if (hands.size() > 0) {
			for (int h=0; h<hands.size(); h++){ // For each hand
				Hand & hand = hands[h];
				if (hand.isValid()){
					
					FingerList fingers = hand.fingers();
					for (int f=0; f<fingers.count(); f++){ // For each finger
						const Finger & finger = fingers[f];
						if (finger.isValid()){
							Finger::Type fingerType = finger.type();

							PolygonObject *Poly = NB->Polys[f];
							Poly->OrigX = 0;
							Poly->OrigY = 0;
							int polyPointCount = 0;
							float screenX, screenY;
							float boneScreenX, boneScreenY;
							
							
							for (int b=0; b<4; b++) { // For each bone
								Bone::Type boneType = static_cast<Bone::Type>(b);
								Bone bone = finger.bone(boneType);
								
								if (bone.isValid()){
									ofPoint bonePt0 = leap.getofPoint ( bone.prevJoint());
									ofPoint bonePt1 = leap.getofPoint ( bone.nextJoint());
									
									if (b == 0){
										ofVec3f bonePtVec3f = ofVec3f(bonePt0.x, bonePt0.y, bonePt0.z);
										ofVec3f screenPtVec3f = screenProjector.getScreenCoordinateOfWorldPosition (bonePtVec3f);
										screenX = screenPtVec3f.x * ((bWorkAtHalfScale) ? 0.5:1.0);
										screenY = screenPtVec3f.y * ((bWorkAtHalfScale) ? 0.5:1.0);
										boneScreenX = ofMap(screenX, 0, nbody_w, -1,1);
										boneScreenY = ofMap(screenY, 0, nbody_h, -1,1);
										if (bone.length() < 0.0001){
											boneScreenX -= 0.001;
											boneScreenY -= 0.001;
										}
										Poly->oPts[polyPointCount*2]    = boneScreenX;
										Poly->oPts[polyPointCount*2+1]  = boneScreenY;
										polyPointCount++;
									}
									
									// While we're here, Deal with color; use color from 2nd joint.
									if (b == 1){
										ofVec3f col = getColorDiagnostically (fingerType, boneType, bonePt0, bonePt1);
										NB->siteColors[f]->r = col.x / 255.0; // NB->siteColors are in range 0..1.
										NB->siteColors[f]->g = col.y / 255.0;
										NB->siteColors[f]->b = col.z / 255.0;
									}
									
									ofVec3f bonePtVec3f = ofVec3f(bonePt1.x, bonePt1.y, bonePt1.z);
									ofVec3f screenPtVec3f = screenProjector.getScreenCoordinateOfWorldPosition (bonePtVec3f);
									screenX = screenPtVec3f.x * ((bWorkAtHalfScale) ? 0.5:1.0);
									screenY = screenPtVec3f.y * ((bWorkAtHalfScale) ? 0.5:1.0);
									boneScreenX = ofMap(screenX, 0, nbody_w, -1,1);
									boneScreenY = ofMap(screenY, 0, nbody_h, -1,1);
									Poly->oPts[polyPointCount*2]    = boneScreenX;
									Poly->oPts[polyPointCount*2+1]  = boneScreenY;
									polyPointCount++;
										
									
								}
							}
							
							
							NB->Polys[f]->outsideUpdate();
							
							
						}
					}
				}
			}
		}
		updateVoronoi();
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawFrame (ofxLeapMotion & leap){
	
	// If hand(s) exist in the frame,
	// Get the vector of Hands from ofxLeap.
	vector <Hand> hands = leap.getLeapHands();
	if (hands.size() > 0) {
		
		// For each hand,
		for (int h=0; h<hands.size(); h++){
			
			// Get the current hand
			Hand & hand = hands[h];
			drawHand (hand,leap);
		}
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawHand (Hand & hand, ofxLeapMotion & leap){
	if (hand.isValid()){
	
		ofDisableSmoothing();
		//glEnable(GL_DEPTH);
		//glEnable(GL_DEPTH_TEST);
		
		drawFingers (hand,leap);
		if (!bDrawDiagnosticColors){
			drawPalm (hand,leap);
		}
		drawArm (hand,leap);
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawFingers (Hand &hand, ofxLeapMotion &leap){
	
	// For each finger in the Hand,
	FingerList fingers = hand.fingers();
	for (int f=0; f<fingers.count(); f++){
		
		// Get the current finger, and it's type (index, thumb, etc.);
		const Finger & finger = fingers[f];
		if (finger.isValid()){
			
			drawFinger(finger,leap);
			
		} //end if finger isValid()
	} // end for each finger
}

//--------------------------------------------------------------
void LeapVisualizer::drawFinger (const Finger &finger, ofxLeapMotion &leap){
	
	if (finger.isValid()){
		
		// For every bone (i.e. phalange) in the finger,
		for (int b=0; b<4; b++) {
			
			// Get each bone;
			Bone::Type boneType = static_cast<Bone::Type>(b);
			Bone bone = finger.bone(boneType);
			if (bone.isValid()){
				
				// Don't consider zero-length bones, such as the Thumb's metacarpal.
				if (bone.length() > 0){
					drawBone (finger, bone,leap);
					
				} // end if boneLength
			} // end if bone isValid()
		} // end for each bone
		
		if (bDrawSimple){
			// Draw the fingertip, which is an extra point within the last phalange.
			ofSetColor(ofColor::white);
			ofPoint fingerTipPt = leap.getofPoint ( finger.tipPosition() );
			ofDrawSphere(fingerTipPt, finger.width() * 0.05);
		}
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawBone (const Finger &finger, Bone &bone, ofxLeapMotion &leap){
	
	Finger::Type fingerType = finger.type();
	Bone::Type   boneType   = bone.type();
	
	// The Leap returns data in millimeters.
	ofPoint bonePt0 = leap.getofPoint ( bone.prevJoint());
	ofPoint bonePt1 = leap.getofPoint ( bone.nextJoint());
	float boneThickness = bone.width();
	
	// ofPoint bonePtC = leap.getofPoint ( bone.center()); // works, but:
	ofPoint bonePtC = (bonePt0 + bonePt1)/2.0;
	
	if (bDrawSimple){
		// Draw a simple white skeleton.
		ofSetColor(ofColor::white);
		ofLine(bonePt0, bonePt1);
		ofDrawSphere(bonePt0, boneThickness * 0.15);
		ofDrawSphere(bonePt1, boneThickness * 0.15);
		ofDrawSphere(bonePtC, boneThickness * 0.05);
		
	} else {
		// Draw a colored cylinder with double-sphere caps.
		float cylinderRadius = boneThickness/2.0;
		
		if (bDrawDiagnosticColors){
			ofVec3f col = getColorDiagnostically (fingerType, boneType, bonePt0, bonePt1);
			ofSetColor (col.x, col.y, col.z);
			cylinderRadius *= diagnosticFingerScaling;
		} else {
			setColorByFinger (fingerType, boneType);
		}
		drawOrientedCylinder (bonePt0, bonePt1, cylinderRadius);
		ofDrawSphere(bonePt0, cylinderRadius);
		ofDrawSphere(bonePt1, cylinderRadius);
	}
}




//--------------------------------------------------------------
void LeapVisualizer::drawPalm (Hand &hand, ofxLeapMotion &leap){
	// This draws the palm as a gray region.
	
	// Collect the palm vertices into an ofMesh.
	ofMesh palmMesh;
	int nVertices = 0;
	float averageBoneWidth = 0;
	
	// For each finger,
	FingerList fingers = hand.fingers();
	for (int f=0; f<fingers.count(); f++){
		
		// Get the current finger, and it's type (index, thumb, etc.);
		const Finger & finger = fingers[f];
		if (finger.isValid()){
			
			Finger::Type fingerType = finger.type();
			Bone bone;
			if (fingerType == Finger::TYPE_THUMB){
				bone = finger.bone(Bone::TYPE_PROXIMAL);
			} else {
				bone = finger.bone(Bone::TYPE_METACARPAL);
			}
			
			// If we've found the bones we want, add their vertices to the mesh.
			if (bone.isValid()){
				float boneLength = bone.length();
				if (boneLength > 0){
					
					ofPoint pt0 = leap.getofPoint ( bone.prevJoint());
					ofPoint pt1 = leap.getofPoint ( bone.nextJoint());
					
					palmMesh.addVertex(pt0);
					palmMesh.addVertex(pt1);
					
					averageBoneWidth += bone.width();
					nVertices += 2;
				}
			}
		}
	}
	averageBoneWidth /= (nVertices/2);
	
	// Render the palm as a triangle strip surface,
	// (optionally) bordered by cylinders.
	if (nVertices > 3){
		
		if (bDrawDiagnosticColors && !bDrawSimple){
			ofSetColor(ID_PALM*32);
		} else {
			ofSetColor(ofColor::gray);
		}
			
		// Draw the palm as a mesh of triangles.
		int nPalmMeshVertices = palmMesh.getNumVertices();
		for (int i=0; i<(nPalmMeshVertices-2); i++){
			palmMesh.addTriangle(i, i+1, i+2);
		}
		
		
		palmMesh.drawFaces();
		
		// Add optional cylinders.
		if (!bDrawSimple){
			float rad = averageBoneWidth / 2.0;
			
			if (nPalmMeshVertices == 10){
				for (int i=0; i<4; i++){
					ofVec3f p0 = palmMesh.getVertex( i   *2);
					ofVec3f p1 = palmMesh.getVertex((i+1)*2);
					drawOrientedCylinder (p0, p1, 10);
					ofDrawSphere(p0, rad);
					ofDrawSphere(p1, rad);
				}
				
				for (int i=0; i<4; i++){
					ofVec3f p0 = palmMesh.getVertex( i   *2 + 1);
					ofVec3f p1 = palmMesh.getVertex((i+1)*2 + 1);
					drawOrientedCylinder (p0, p1, 10);
					ofDrawSphere(p0, rad);
					ofDrawSphere(p1, rad);
				}
			}
		}
	
	
		
		
		
	}
}


//--------------------------------------------------------------
void LeapVisualizer::drawArm (Hand & hand,ofxLeapMotion & leap){
	
	// Draw the wrist and elbow points.
	Arm arm = hand.arm();
	if (arm.isValid()){
		
		ofPoint handPt   = leap.getofPoint ( hand.palmPosition());
		ofPoint handNorm = leap.getofPoint ( hand.palmNormal());
		ofPoint wristPt  = leap.getofPoint ( arm.wristPosition());
		ofPoint elbowPt  = leap.getofPoint ( arm.elbowPosition());
		
		float basisLen = 50.0;
		
		
		if (bDrawSimple){
			ofSetColor(ofColor::white);
			ofDrawSphere(handPt,  8.0);
			ofDrawSphere(wristPt, 8.0);
			ofDrawSphere(elbowPt, 8.0);
			
			ofLine(handPt, wristPt);
			ofLine(wristPt, elbowPt);
			ofLine(handPt, handPt+ basisLen*handNorm);
			ofDrawSphere(handPt+ basisLen*handNorm, 2.0);
			
			// draw the rotation vectors of the hand.
			{
				Leap::Matrix handMatrix = hand.basis();
				ofPoint handBasisX = leap.getofPoint( handMatrix.xBasis);
				ofPoint handBasisY = leap.getofPoint( handMatrix.yBasis);
				ofPoint handBasisZ = leap.getofPoint( handMatrix.zBasis);
				
				glLineWidth(2.0);
				ofSetColor(ofColor::red  );	ofLine(handPt, handPt + basisLen*handBasisX);
				ofSetColor(ofColor::green);	ofLine(handPt, handPt + basisLen*handBasisY);
				ofSetColor(ofColor::blue );	ofLine(handPt, handPt + basisLen*handBasisZ);
				glLineWidth(1.0);
				
				// draw the identity of the hand (left or right)
				string whichHandString = "RIGHT";
				if (hand.isLeft()){
					whichHandString = "LEFT";
				}
				// float handConfidence = hand.confidence();
				// whichHandString += " " + ofToString(handConfidence);
				
				ofSetColor(ofColor::white);
				ofDrawBitmapString(whichHandString, (handPt + (basisLen*1.2)*handBasisY));
			}
			
			// draw the rotation vectors of the arm.
			{
				Leap::Matrix armMatrix = arm.basis();
				ofPoint armBasisX = leap.getofPoint( armMatrix.xBasis);
				ofPoint armBasisY = leap.getofPoint( armMatrix.yBasis);
				ofPoint armBasisZ = leap.getofPoint( armMatrix.zBasis);
				
				glLineWidth(2.0);
				ofSetColor(ofColor::red  );	ofLine(wristPt, wristPt + basisLen*armBasisX);
				ofSetColor(ofColor::green);	ofLine(wristPt, wristPt + basisLen*armBasisY);
				ofSetColor(ofColor::blue );	ofLine(wristPt, wristPt + basisLen*armBasisZ);
				glLineWidth(1.0);
			}
			
		} else {
			
			// Draw a cylinder between two points, properly oriented in space.
			float armWidth = arm.width();
			float dx = wristPt.x - elbowPt.x;
			float dy = wristPt.y - elbowPt.y;
			float dz = wristPt.z - elbowPt.z;
			float dh = sqrt(dx*dx + dy*dy + dz*dz);
			
			ofPushMatrix();
			{
				ofTranslate( (elbowPt.x+wristPt.x)/2, (elbowPt.y+wristPt.y)/2, (elbowPt.z+wristPt.z)/2 );
				
				float theta =   90 - RAD_TO_DEG * asin(dz/dh);
				float phi   =        RAD_TO_DEG * atan2(dy,dx);
				ofRotate(phi,   0,0,1);
				ofRotate(theta, 0,1,0);
				ofRotate(90,	1,0,0);
				
				// Get the arm Matrix, which provides its orthogonal basis vectors.
				Leap::Matrix armMatrix = arm.basis();
				ofPoint armBasisY = leap.getofPoint( armMatrix.yBasis);
				float ax = armBasisY.x;
				float ay = armBasisY.y;
				float az = armBasisY.z;
				
				// Compute the longitudinal rotation of the arm.
				// Sheesh, I really need to learn 3D matrix math.
				ofNode armBasisYNode;
				armBasisYNode.setPosition(armBasisY);
				armBasisYNode.rotateAround(0-   phi, ofVec3f(0,0,1), ofVec3f(0,0,0));
				armBasisYNode.rotateAround(0- theta, ofVec3f(0,1,0), ofVec3f(0,0,0));
				armBasisYNode.rotateAround(0-    90, ofVec3f(1,0,0), ofVec3f(0,0,0));
				ofPoint newArmBasisY = armBasisYNode.getPosition();
				float armRotation = RAD_TO_DEG * atan2f(newArmBasisY.z, newArmBasisY.x);
				
				ofPushMatrix();
				{
					ofRotate(armRotation, 0,-1,0);
					float armThicknessRatio = 0.6;
					glScalef(armThicknessRatio, 1.0, 1.0);
					if (bDrawDiagnosticColors){
						ofSetColor(ID_WRIST*32);
					} else {
						ofSetColor(ofColor::magenta);
					}
					
					// Oblate arm cylinder
					ofDrawCylinder (armWidth/2.0, dh);
					
					// Wrist endcap
					ofPushMatrix();
					ofTranslate(ofPoint(0, dh/2,0));
					glScalef(1.0, armThicknessRatio, 1.0);
					ofDrawSphere(armWidth/2.0);
					ofPopMatrix();
					
					// Elbow endcap
					ofPushMatrix();
					ofTranslate(ofPoint(0, -dh/2,0));
					glScalef(1.0, armThicknessRatio, 1.0);
					ofDrawSphere(armWidth/2.0);
					ofPopMatrix();
					
				} // Close popMatrix
				ofPopMatrix();
			} // Close popMatrix
			ofPopMatrix();
		} // Close if !drawSimple
	} // Close if arm isValid
}


//--------------------------------------------------------------
void LeapVisualizer::drawFrameFromXML(int whichFrame){
	
	int nFrameTags = XML.getNumTags("FRAME");
	if ((whichFrame >= 0) && (whichFrame < nFrameTags)){
		
		//we push into the which'h FRAME tag; this temporarily treats the tag as the document root.
		XML.pushTag("FRAME", whichFrame);
		
		int nHandTags = XML.getNumTags("H");
		if (nHandTags > 0){
			for (int h=0; h<nHandTags; h++){
				drawHandFromXML(h);
			}
		}
		
		//this pops us out of the FRAME tag, sets the root back to the xml document
		XML.popTag();
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawHandFromXML (int whichHand){
	XML.pushTag("H", whichHand);
	//glEnable(GL_DEPTH);
	//glEnable(GL_DEPTH_TEST);
	
	drawFingersFromXML();
	if (!bDrawDiagnosticColors){
		drawPalmFromXML();
	}
	drawArmFromXML();
	
	XML.popTag();
}


//--------------------------------------------------------------
void LeapVisualizer::drawVoronoiFrameFromXML(int whichFrame){
	if (bEnableVoronoiRendering){
		int nFrameTags = XML.getNumTags("FRAME");
		if ((whichFrame >= 0) && (whichFrame < nFrameTags)){
			XML.pushTag("FRAME", whichFrame);
			int nHandTags = XML.getNumTags("H");
			if (nHandTags > 0){
				for (int h=0; h<nHandTags; h++){
					XML.pushTag("H", h);
					int nFingerTags = XML.getNumTags("F");
					if (nFingerTags > 0){
						for (int f=0; f<nFingerTags; f++){
							XML.pushTag("F", f);
							feedXMLFingerPointsToVoronoi(f);
							XML.popTag();
						}
					}
					XML.popTag();
				}
			}
			XML.popTag();
		}
		updateVoronoi();
	}
}


//--------------------------------------------------------------
void LeapVisualizer::feedXMLFingerPointsToVoronoi (int whichFinger){
	if (bEnableVoronoiRendering && bProjectorSet){
		
		if (whichFinger < NUM_NBODIES){
			
			PolygonObject *Poly = NB->Polys[whichFinger];
			Poly->OrigX = 0;
			Poly->OrigY = 0;
			
			Finger::Type fingerType =  (Finger::Type) XML.getValue("TYPE", 0);
			
			int nBoneTags = XML.getNumTags("B");
			if (nBoneTags > 0){
				
				ofPoint bonePt0;
				ofPoint bonePt1;
				float screenX;
				float screenY;
				float boneScreenX = 0;
				float boneScreenY = 0;
				int polyPointCount = 0;
				
				for (int b=0; b<nBoneTags; b++){
					XML.pushTag("B", b);
					Bone::Type	boneType =  (Bone::Type) XML.getValue("TYPE", 0);
					
					// Deal with initial ("Q" tag) point.
					if (XML.getNumTags("Q") > 0){
						bonePt0 = ofPoint(XML.getValue("Q:X",0.0), XML.getValue("Q:Y",0.0), XML.getValue("Q:Z",0.0));
						ofVec3f bonePtVec3f = ofVec3f(bonePt0.x, bonePt0.y, bonePt0.z);
						ofVec3f screenPtVec3f = screenProjector.getScreenCoordinateOfWorldPosition (bonePtVec3f);
						screenX = screenPtVec3f.x * ((bWorkAtHalfScale) ? 0.5:1.0);
						screenY = screenPtVec3f.y * ((bWorkAtHalfScale) ? 0.5:1.0);
						boneScreenX = ofMap(screenX, 0, nbody_w, -1,1);
						boneScreenY = ofMap(screenY, 0, nbody_h, -1,1);
						Poly->oPts[polyPointCount*2]    = boneScreenX;
						Poly->oPts[polyPointCount*2+1]  = boneScreenY;
						polyPointCount++;
					}
					
					// Deal with intermediate ("P" tag) points.
					bonePt1 = ofPoint(XML.getValue("P:X",0.0), XML.getValue("P:Y",0.0), XML.getValue("P:Z",0.0));
					ofVec3f bonePtVec3f = ofVec3f(bonePt1.x, bonePt1.y, bonePt1.z);
					ofVec3f screenPtVec3f = screenProjector.getScreenCoordinateOfWorldPosition (bonePtVec3f);
					screenX = screenPtVec3f.x * ((bWorkAtHalfScale) ? 0.5:1.0);
					screenY = screenPtVec3f.y * ((bWorkAtHalfScale) ? 0.5:1.0);
					boneScreenX = ofMap(screenX, 0, nbody_w, -1,1);
					boneScreenY = ofMap(screenY, 0, nbody_h, -1,1);
					Poly->oPts[polyPointCount*2]    = boneScreenX;
					Poly->oPts[polyPointCount*2+1]  = boneScreenY;
					polyPointCount++;

					// Deal with color; use color from second (palm interior) joint.
					if (b == 1){
						ofVec3f col = getColorDiagnostically (fingerType, boneType, bonePt0, bonePt1);
						NB->siteColors[whichFinger]->r = col.x / 255.0; // NB->siteColors are in range 0..1.
						NB->siteColors[whichFinger]->g = col.y / 255.0;
						NB->siteColors[whichFinger]->b = col.z / 255.0;
					}
					
					bonePt0 = bonePt1;
					XML.popTag();
				}
				
				// Special case for final thumb point (which needs an extra copy of final point)
				if (fingerType == Finger::TYPE_THUMB){
					// We subtract a smidge of 0.001 so that there's no 1/0 numerical error at the endpoint.
					Poly->oPts[polyPointCount*2  ]  = Poly->oPts[(polyPointCount-1)*2  ] - 0.001;
					Poly->oPts[polyPointCount*2+1]  = Poly->oPts[(polyPointCount-1)*2+1] - 0.001;
				}
				
			}
					
			NB->Polys[whichFinger]->outsideUpdate();
		}
	}

}


//--------------------------------------------------------------
void LeapVisualizer::drawFingersFromXML(){
	
	int nFingerTags = XML.getNumTags("F");
	if (nFingerTags > 0){
		for (int f=0; f<nFingerTags; f++){
			XML.pushTag("F", f);
			drawFingerFromXML();
			XML.popTag();
		}
	}
}

//--------------------------------------------------------------
void LeapVisualizer::drawFingerFromXML(){
	
	Finger::Type	fingerType =  (Finger::Type) XML.getValue("TYPE", 0);
	float			fingerWidth = XML.getValue("WIDTH", 0.0);
	ofPoint			fingerTipPt = ofPoint(XML.getValue("T:X",0.0), XML.getValue("T:Y",0.0), XML.getValue("T:Z",0.0));
	
	int nBoneTags = XML.getNumTags("B");
	if (nBoneTags > 0){
		
		ofPoint bonePt0;
		ofPoint bonePt1;
		ofPoint bonePtC;
		for (int b=0; b<nBoneTags; b++){
			XML.pushTag("B", b);
			Bone::Type	boneType =  (Bone::Type) XML.getValue("TYPE", 0);
			if (XML.getNumTags("Q") > 0){
                bonePt0 = ofPoint(XML.getValue("Q:X",0.0), XML.getValue("Q:Y",0.0), XML.getValue("Q:Z",0.0));
			}
			bonePt1 = ofPoint(XML.getValue("P:X",0.0), XML.getValue("P:Y",0.0), XML.getValue("P:Z",0.0));
			bonePtC = (bonePt0 + bonePt1)/2.0;
			
			if (bDrawSimple){
				// Draw a simple skeleton.
				ofSetColor(ofColor::orange);
				ofLine(bonePt0, bonePt1);
				ofDrawSphere(bonePt0, fingerWidth * 0.15);
				ofDrawSphere(bonePt1, fingerWidth * 0.15);
				ofDrawSphere(bonePtC, fingerWidth * 0.05);
				
			} else {
				// Draw a colored cylinder with double-sphere caps.
				float cylinderRadius = fingerWidth/2.0;
				
				if (bDrawDiagnosticColors){
					ofVec3f col = getColorDiagnostically (fingerType, boneType, bonePt0, bonePt1);
					ofSetColor (col.x, col.y, col.z);
					cylinderRadius *= diagnosticFingerScaling;
				} else {
					setColorByFinger (fingerType, boneType);
				}
				drawOrientedCylinder (bonePt0, bonePt1, cylinderRadius);
				ofDrawSphere(bonePt0, cylinderRadius);
				ofDrawSphere(bonePt1, cylinderRadius);
			}

			bonePt0 = bonePt1;
			XML.popTag();
		}
	}
}


//--------------------------------------------------------------
void LeapVisualizer::drawPalmFromXML(){
	
	int nFingerTags = XML.getNumTags("F");
	if (nFingerTags > 0){
		
		ofMesh palmMesh;
		int nVertices = 0;
		float averageFingerWidth = 0;
		
		for (int f=0; f<nFingerTags; f++){
			XML.pushTag("F", f);
			float fingerWidth = XML.getValue("WIDTH", 0.0);
			averageFingerWidth += fingerWidth;
			
			if (XML.getNumTags("B") > 0){
				XML.pushTag("B", 0);
				palmMesh.addVertex( ofPoint(XML.getValue("Q:X",0.0), XML.getValue("Q:Y",0.0), XML.getValue("Q:Z",0.0)) );
				palmMesh.addVertex( ofPoint(XML.getValue("P:X",0.0), XML.getValue("P:Y",0.0), XML.getValue("P:Z",0.0)) );
				nVertices += 2;
				XML.popTag();
			}
			XML.popTag();
		}
		
		if (nVertices > 3){
			if (bDrawSimple){
				ofSetColor(ofColor::brown);
			} else {
				if (bDrawDiagnosticColors){
					ofSetColor(ID_PALM*32);
				} else {
					ofSetColor(ofColor::gray);
				}
			}
			
			// Draw the palm as a mesh of triangles.
			int nPalmMeshVertices = palmMesh.getNumVertices();
			for (int i=0; i<(nPalmMeshVertices-2); i++){
				palmMesh.addTriangle(i, i+1, i+2); }
			palmMesh.drawFaces();
			
			if (!bDrawSimple){
				averageFingerWidth /= nFingerTags;
				if (nPalmMeshVertices == 10){
					float rad = averageFingerWidth / 2.0;
					for (int i=0; i<4; i++){
						ofVec3f p0 = palmMesh.getVertex( i   *2);
						ofVec3f p1 = palmMesh.getVertex((i+1)*2);
						drawOrientedCylinder (p0, p1, 10);
						ofDrawSphere(p0, rad);
						ofDrawSphere(p1, rad);
					}
					for (int i=0; i<4; i++){
						ofVec3f p0 = palmMesh.getVertex( i   *2 + 1);
						ofVec3f p1 = palmMesh.getVertex((i+1)*2 + 1);
						drawOrientedCylinder (p0, p1, 10);
						ofDrawSphere(p0, rad);
						ofDrawSphere(p1, rad);
					}
				}
			}
		}
	}
}


//--------------------------------------------------------------
void LeapVisualizer::drawArmFromXML(){
	
	float armWidth = XML.getValue("AW", 0.0);
	float basisLen = 50.0;
	
	ofPoint	handPt    = ofPoint(XML.getValue("PM:X",0.0), XML.getValue("PM:Y",0.0), XML.getValue("PM:Z",0.0));	// palm
	ofPoint	wristPt   = ofPoint(XML.getValue("W:X",0.0),  XML.getValue("W:Y",0.0),  XML.getValue("W:Z",0.0));	// wrist
	ofPoint	elbowPt   = ofPoint(XML.getValue("E:X",0.0),  XML.getValue("E:Y",0.0),  XML.getValue("E:Z",0.0));	// elbow
	ofPoint	handNorm  = ofPoint(XML.getValue("PN:X",0.0), XML.getValue("PN:Y",0.0), XML.getValue("PN:Z",0.0));	// palm normal
	
	//Hand basis matrix
	XML.pushTag("HM", 0);
    ofPoint handBasisX = ofPoint(XML.getValue("XX",0.0), XML.getValue("XY",0.0), XML.getValue("XZ",0.0));
    ofPoint handBasisY = ofPoint(XML.getValue("YX",0.0), XML.getValue("YY",0.0), XML.getValue("YZ",0.0));
    ofPoint handBasisZ = ofPoint(XML.getValue("ZX",0.0), XML.getValue("ZY",0.0), XML.getValue("ZZ",0.0));
	XML.popTag();
	
	// Arm basis matrix
	XML.pushTag("AM", 0);
    ofPoint armBasisX = ofPoint(XML.getValue("XX",0.0), XML.getValue("XY",0.0), XML.getValue("XZ",0.0));
    ofPoint armBasisY = ofPoint(XML.getValue("YX",0.0), XML.getValue("YY",0.0), XML.getValue("YZ",0.0));
    ofPoint armBasisZ = ofPoint(XML.getValue("ZX",0.0), XML.getValue("ZY",0.0), XML.getValue("ZZ",0.0));
	XML.popTag();
	
    
	// Draw the wrist and elbow points.
	if (bDrawSimple){
		ofSetColor(ofColor::orange);
		ofDrawSphere(handPt,  8.0);
		ofDrawSphere(wristPt, 8.0);
		ofDrawSphere(elbowPt, 8.0);
		
		ofLine(handPt, wristPt);
		ofLine(wristPt, elbowPt);
		ofLine(handPt, handPt+ basisLen*handNorm);
		ofDrawSphere(handPt+ basisLen*handNorm, 2.0);
		
		glLineWidth(2.0);
        // draw the rotation vectors of the hand.
        ofSetColor(ofColor::red  );	ofLine(handPt, handPt + basisLen*handBasisX);
        ofSetColor(ofColor::green);	ofLine(handPt, handPt + basisLen*handBasisY);
        ofSetColor(ofColor::blue );	ofLine(handPt, handPt + basisLen*handBasisZ);
		
        // draw the rotation vectors of the arm.
        ofSetColor(ofColor::red  );	ofLine(wristPt, wristPt + basisLen*armBasisX);
        ofSetColor(ofColor::green);	ofLine(wristPt, wristPt + basisLen*armBasisY);
        ofSetColor(ofColor::blue );	ofLine(wristPt, wristPt + basisLen*armBasisZ);
		glLineWidth(1.0);
		
		string handType = XML.getValue("TYPE", "RIGHT");
		ofSetColor(ofColor::orange);
		ofDrawBitmapString(handType, (handPt + (basisLen*1.2)*handBasisY));
		
        
        
	} else {
		
		// Draw a cylinder between two points, properly oriented in space.
		float dx = wristPt.x - elbowPt.x;
		float dy = wristPt.y - elbowPt.y;
		float dz = wristPt.z - elbowPt.z;
		float dh = sqrt(dx*dx + dy*dy + dz*dz);
		
		ofPushMatrix();
		{
			ofTranslate( (elbowPt.x+wristPt.x)/2, (elbowPt.y+wristPt.y)/2, (elbowPt.z+wristPt.z)/2 );
			
			float theta =   90 - RAD_TO_DEG * asin(dz/dh);
			float phi   =        RAD_TO_DEG * atan2(dy,dx);
			ofRotate(phi,   0,0,1);
			ofRotate(theta, 0,1,0);
			ofRotate(90,	1,0,0);
			
			// Get the arm Matrix, which provides its orthogonal basis vectors.
			float ax = armBasisY.x;
			float ay = armBasisY.y;
			float az = armBasisY.z;
			
			// Compute the longitudinal rotation of the arm
			ofNode armBasisYNode;
			armBasisYNode.setPosition(armBasisY);
			armBasisYNode.rotateAround(0-   phi, ofVec3f(0,0,1), ofVec3f(0,0,0));
			armBasisYNode.rotateAround(0- theta, ofVec3f(0,1,0), ofVec3f(0,0,0));
			armBasisYNode.rotateAround(0-    90, ofVec3f(1,0,0), ofVec3f(0,0,0));
			ofPoint newArmBasisY = armBasisYNode.getPosition();
			float armRotation = RAD_TO_DEG * atan2f(newArmBasisY.z, newArmBasisY.x);
			
			ofPushMatrix();
			{
				ofRotate(armRotation, 0,-1,0);
				float armThicknessRatio = 0.6;
				glScalef(armThicknessRatio, 1.0, 1.0);
				if (bDrawDiagnosticColors){
					ofSetColor(ID_WRIST*32);
				} else {
					ofSetColor(ofColor::magenta);
				}
				
				// Oblate arm cylinder
				ofDrawCylinder (armWidth/2.0, dh);
				
				// Wrist endcap
				ofPushMatrix();
				ofTranslate(ofPoint(0, dh/2,0));
				glScalef(1.0, armThicknessRatio, 1.0);
				ofDrawSphere(armWidth/2.0);
				ofPopMatrix();
				
				// Elbow endcap
				ofPushMatrix();
				ofTranslate(ofPoint(0, -dh/2,0));
				glScalef(1.0, armThicknessRatio, 1.0);
				ofDrawSphere(armWidth/2.0);
				ofPopMatrix();
				
			} // Close popMatrix
			ofPopMatrix();
		} // Close popMatrix
		ofPopMatrix();
	} // Close if !drawSimple
    
}




