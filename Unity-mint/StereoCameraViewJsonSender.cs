using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

using interop;
using UnityEngine.UIElements;
using Valve.VR;

// collects camera pose for left and right eye and makes them available as StereoCameraView in Json format
// optionally computes the relative pose of camera to some dataset object (if the renderer producing images does not support model matrices)
public class StereoCameraViewJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "StereoCameraView";

    public bool CameraPositionRelative = false;
    public GameObject VrCameraPose = null;
    public float predictionValue = 0.0f;
    public GameObject offsetSource;
    public bool usePrediction = false;
    public bool useSmoothing = false;
    public float rotSmooth = 1.0f;

    private GameObject relativeCameraPositionL = null;
    private GameObject relativeCameraPositionR = null;
    private GameObject PositionRelativeTo = null;

    private GameObject vrEyePositionL = null;
    private GameObject vrEyePositionR = null;
    
    private TrackedDevicePose_t[] _poses = new TrackedDevicePose_t[OpenVR.k_unMaxTrackedDeviceCount];

    public void Start()
    {
        if(CameraPositionRelative)
        {
            if(PositionRelativeTo == null)
                PositionRelativeTo = this.gameObject;

            if(relativeCameraPositionL == null)
            {
                relativeCameraPositionL = new GameObject("relativeStereoCameraPositionL");
                relativeCameraPositionR = new GameObject("relativeStereoCameraPositionR");
                relativeCameraPositionL.transform.SetParent(PositionRelativeTo.transform);
                relativeCameraPositionR.transform.SetParent(PositionRelativeTo.transform);
            }

            if(VrCameraPose == null) {
                Debug.LogError("StereoCameraViewJsonSender: for relative Camera positioning, provide a VR Camera (Vr Camera Pose)");
            }
            else {
                var camParent = VrCameraPose.transform.parent;
                if(camParent == null)
                    camParent = VrCameraPose.transform;

                // we want to track the global position of the left/right eye XR nodes, as the API gives us only the local coords.
                // we can not directly append the VR eye nodes to the camera for global position tracking,
                // as the camera itself also gets transformed as an XR node.
                // instead, attach the eyes to parent of the camera. this way, we should get the global pose of eye XR nodes.
                vrEyePositionL = new GameObject("vrEyePositionL");
                vrEyePositionR = new GameObject("vrEyePositionR");
                vrEyePositionL.transform.SetParent(camParent);
                vrEyePositionR.transform.SetParent(camParent);
            }
        }
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString()
    {
        StereoCameraView cc;
        try
        {
            cc = StereoCameraViewFromXR();
        }
        catch (NullReferenceException)
        {
            cc = StereoCameraViewFromCamera();
        }
        string json = cc.json();
        return json;
	}

    public bool hasChanged()
    {
        return true;
    }

    StereoCameraView StereoCameraViewFromCamera()
    {
        StereoCameraView scv;
        
        GameObject vrCamera = GameObject.Find("Camera");
        
        setEyeView(out scv.leftEyeView, vrCamera.transform.position, vrCamera.transform.rotation);
        setEyeView(out scv.rightEyeView, vrCamera.transform.position, vrCamera.transform.rotation);
        
        return scv;
    }
    
    StereoCameraView StereoCameraViewFromXR()
    {
        StereoCameraView scv;

        // TODO opt 1: position relative to VR origin
        // => solved via parent GameObject?
        Vector3 eyePosL;
        Vector3 eyePosR;
        if (usePrediction)
        {
            OpenVR.System.GetDeviceToAbsoluteTrackingPose(ETrackingUniverseOrigin.TrackingUniverseStanding,
                predictionValue, _poses);
            SteamVR_Utils.RigidTransform leftT =
                new SteamVR_Utils.RigidTransform(OpenVR.System.GetEyeToHeadTransform(EVREye.Eye_Left));
            SteamVR_Utils.RigidTransform rightT =
                new SteamVR_Utils.RigidTransform(OpenVR.System.GetEyeToHeadTransform(EVREye.Eye_Right));
            SteamVR_Utils.RigidTransform headT = new SteamVR_Utils.RigidTransform(_poses[0].mDeviceToAbsoluteTracking);
            SteamVR_Utils.RigidTransform leftPos = leftT * headT;
            SteamVR_Utils.RigidTransform rightPos = rightT * headT;
           eyePosL = leftPos.pos;
           eyePosR = rightPos.pos;
        }
        else
        {
            if (useSmoothing)
            {
                eyePosL = InputTracking.GetLocalPosition(XRNode.LeftEye).Round(3); //- vrOrigin.transform.position;
                eyePosR = InputTracking.GetLocalPosition(XRNode.RightEye).Round(3); //- vrOrigin.transform.position;
            }
            else
            {
                eyePosL = InputTracking.GetLocalPosition(XRNode.LeftEye); //- vrOrigin.transform.position;
                eyePosR = InputTracking.GetLocalPosition(XRNode.RightEye); //- vrOrigin.transform.position;
            }
        }

        // TODO opt 2: rotate camera around dataset, if dataset can not be rotated in renderer
        // => additional Rotation to be added to dataset GameObject in Unity beforehand?
        Quaternion eyeRotL = InputTracking.GetLocalRotation(XRNode.LeftEye) ; // * additionalRotation
        Quaternion eyeRotR = InputTracking.GetLocalRotation(XRNode.RightEye); // * additionalRotation

        if(CameraPositionRelative)
        {
            if(VrCameraPose)
            {
                // InputTracking only gives us 'local coordinates' relative to parent object
                // get global position and orientation of left/right eye XR nodes
                vrEyePositionL.transform.localPosition = eyePosL;
                vrEyePositionR.transform.localPosition = eyePosR;
                if (useSmoothing)
                {
                    vrEyePositionL.transform.localRotation = Quaternion.Lerp(vrEyePositionL.transform.localRotation,eyeRotL, rotSmooth);
                    vrEyePositionR.transform.localRotation = Quaternion.Lerp(vrEyePositionR.transform.localRotation,eyeRotR, rotSmooth);
                }
                else
                {
                    vrEyePositionL.transform.localRotation = eyeRotL;
                    vrEyePositionR.transform.localRotation = eyeRotR;
                }

                // get global pose of node
                eyePosL = vrEyePositionL.transform.position - offsetSource.transform.position;
                eyePosR = vrEyePositionR.transform.position - offsetSource.transform.position;
                eyeRotL = vrEyePositionL.transform.rotation;
                eyeRotR = vrEyePositionR.transform.rotation;
            }

            computeRelativePose(relativeCameraPositionL.transform, ref eyePosL, ref eyeRotL);
            computeRelativePose(relativeCameraPositionR.transform, ref eyePosR, ref eyeRotR);
        }

        setEyeView(out scv.leftEyeView, eyePosL, eyeRotL);
        setEyeView(out scv.rightEyeView, eyePosR, eyeRotR);

        return scv;
    }

    private void computeRelativePose(Transform relativeTransform, ref Vector3 eyePos, ref Quaternion eyeRot)
    {
        // enter global pose into relative dataset transform, retrieve local pose (pose relative to object)
        relativeTransform.SetPositionAndRotation(eyePos, eyeRot);

        eyePos = relativeTransform.localPosition;
        eyeRot = relativeTransform.localRotation;
    }

    private void setEyeView(out CameraView view, Vector3 eyePos, Quaternion eyeRot)
    {
        view.eyePos    = convert.toOpenGL(eyePos);
        view.lookAtPos = convert.toOpenGL(eyePos + (eyeRot * Vector3.forward));
        view.camUpDir  = convert.toOpenGL(eyeRot * Vector3.up);
    }
}
