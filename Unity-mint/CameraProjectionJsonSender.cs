﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Valve.VR;

using interop;

// Returns Camera Projection Parameters as Json string in interop format.
// Projection parameters are read from the camera the script is attached to,
// or from values for the HMD provided by OpenVR.
// This script needs to be attached to a Camera Object.
[RequireComponent(typeof(Camera))]
public class CameraProjectionJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "CameraProjection";
    public bool useHmdParams = true;

    private Camera m_camera = null;

    public void Start()
    {
        m_camera = GetComponent<Camera>();
        //useHmdParams = OpenVR.IsHmdPresent();
    }

    private void printError()
    {
        string objectName = gameObject.name;
        Debug.LogError("CameraProjectionJsonConverter in Object '" + objectName + "' is not attached to a Camera Object!");
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        CameraProjection cp = CameraProjectionFromCamera(m_camera);
        string json = cp.json();
        return json;
	}
	
    private CameraProjection CameraProjectionFromCamera(Camera cam)
    {
        CameraProjection cp;

        if(useHmdParams) {
            setFromHMD(out cp);
        } else {
            setFromCamera(out cp);
        }

        return cp;
    }

    private void setFromCamera(out CameraProjection cp) {
        cp.fieldOfViewY_rad = m_camera.fieldOfView * Mathf.Deg2Rad; // vertical field of view in radians
        cp.nearClipPlane = m_camera.nearClipPlane;
        cp.farClipPlane = m_camera.farClipPlane;
        cp.aspect = m_camera.aspect;
        cp.pixelWidth = (uint)m_camera.pixelWidth;
        cp.pixelHeight = (uint)m_camera.pixelHeight;
    }
    
    private void setFromHMD(out CameraProjection cp) {
        cp.nearClipPlane = m_camera.nearClipPlane;
        cp.farClipPlane = m_camera.farClipPlane;

        cp.pixelWidth = (uint)SteamVR.instance.sceneWidth; // SteamVR gives extended render size for symmetric projection
        cp.pixelHeight = (uint)SteamVR.instance.sceneHeight;

        cp.aspect = SteamVR.instance.aspect;
        cp.fieldOfViewY_rad = SteamVR.instance.fieldOfView * Mathf.Deg2Rad; // vertical field of view in radians

        // for computation of respective values, see SteamVR/Scripts/SteamVR.cs:SteamVR()
        var textureBounds = SteamVR.instance.textureBounds; // 0 -> left; 1 -> right; uMin, uMax, vMin, vMax
        // texture bounds give us area in rendered texture which needs to be stretched onto left/right eye render target
        // TODO: publish texture bounds to correct shader for XR overlay
    }
    
}

class StereoTextureCutoutBounds {
    TextureBounds leftEye;
    TextureBounds rightEye;
}

struct TextureBounds {
    float uMin;
    float uMax;
    float vMin;
    float vMax;
}