using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

using interop;

// collects camera pose for left and right eye and makes them available as StereoCameraView in Json format
// optionally computes the relative pose of camera to some dataset object (if the renderer producing images does not support model matrices)
public class StereoCameraViewJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "StereoCameraView";

    public bool CameraPositionRelative = false;
    public GameObject PositionRelativeTo = null;
    public string relativeModeNameSuffix = "Relative";

    private GameObject relativeCameraPositionL = null;
    private GameObject relativeCameraPositionR = null;

    public void Start()
    {
        if(CameraPositionRelative)
        {
            Name = Name + relativeModeNameSuffix;

            if(PositionRelativeTo == null)
                PositionRelativeTo = this.gameObject;

            if(relativeCameraPositionL == null)
            {
                relativeCameraPositionL = new GameObject("relativeStereoCameraPositionL");
                relativeCameraPositionR = new GameObject("relativeStereoCameraPositionR");

                relativeCameraPositionL.transform.SetParent(PositionRelativeTo.transform);
                relativeCameraPositionR.transform.SetParent(PositionRelativeTo.transform);
            }
        }
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        StereoCameraView cc = StereoCameraViewFromXR();
        string json = cc.json();
        return json;
	}
	
    StereoCameraView StereoCameraViewFromXR()
    {
        StereoCameraView scv;

        // TODO opt 1: position relative to VR origin
        // => solved via parent GameObject?
        Vector3 eyePosL = InputTracking.GetLocalPosition(XRNode.LeftEye) ; //- vrOrigin.transform.position;
        Vector3 eyePosR = InputTracking.GetLocalPosition(XRNode.RightEye); //- vrOrigin.transform.position;

        // TODO opt 2: rotate camera around dataset, if dataset can not be rotated in renderer
        // => additional Rotation to be added to dataset GameObject in Unity beforehand?
        Quaternion eyeRotL = InputTracking.GetLocalRotation(XRNode.LeftEye) ; // * additionalRotation
        Quaternion eyeRotR = InputTracking.GetLocalRotation(XRNode.RightEye); // * additionalRotation

        if(CameraPositionRelative)
        {
            computeRelativePose(relativeCameraPositionL.transform, ref eyePosL, ref eyeRotL);
            computeRelativePose(relativeCameraPositionR.transform, ref eyePosR, ref eyeRotR);
        }

        setEyeView(out scv.leftEyeView, eyePosL, eyeRotL);
        setEyeView(out scv.rightEyeView, eyePosR, eyeRotR);

        return scv;
    }

    private void computeRelativePose(Transform relativeTransform, ref Vector3 eyePos, ref Quaternion eyeRot)
    {
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
