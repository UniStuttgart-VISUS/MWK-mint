using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

using interop;

// collects camera pose for left and right eye and makes them available as StereoCameraView in Json format
public class StereoCameraViewJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "StereoCameraView";

    //private Camera m_camera = null;

    public void Start()
    {
        //m_camera = GetComponent<Camera>();
        //if (m_camera == null)
        //    printError();
    }

    private void printError()
    {
        string objectName = gameObject.name;
        Debug.LogError("StereoCameraViewJsonSender in Object '" + objectName + "' is not attached to a Camera Object!");
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        //if (m_camera == null)
        //{
        //    printError();
        //    return "";
        //}

        StereoCameraView cc = StereoCameraViewFromXR();
        string json = cc.json();
        return json;
	}
	
    StereoCameraView StereoCameraViewFromXR()
    {
        StereoCameraView scv;

        // TODO opt 1: position relative to VR origin
        Vector3 eyePosL = InputTracking.GetLocalPosition(XRNode.LeftEye) ; //- vrOrigin.transform.position;
        Vector3 eyePosR = InputTracking.GetLocalPosition(XRNode.RightEye); //- vrOrigin.transform.position;

        // TODO opt 2: rotate camera around dataset, if dataset can not be rotated in renderer
        Quaternion eyeRotL = InputTracking.GetLocalRotation(XRNode.LeftEye) ; // * additionalRotation
        Quaternion eyeRotR = InputTracking.GetLocalRotation(XRNode.RightEye); // * additionalRotation

        // TODO opt 3: what happens without inviwo positioning?
        //if (!EnvConstants.UseInviwoPositioning)
        //{
        //    vec1 = vec1 / vec1.magnitude;
        //    vec2 = vec2 / vec2.magnitude;
        //}

        //Quaternion addRotation = Quaternion.Inverse(additionalRotationFrom.transform.rotation);
        //if (additionalRotationFrom != null)
        //{
        //    vec1 = RotatePointAroundPivot(vec1, new Vector3(0f, 0f, 0f), addRotation);
        //    vec2 = RotatePointAroundPivot(vec2, new Vector3(0f, 0f, 0f), addRotation);
        //    vecFwd = RotatePointAroundPivot(vecFwd, new Vector3(0f, 0f, 0f), addRotation);
        //    vecUp = RotatePointAroundPivot(vecUp, new Vector3(0f, 0f, 0f), addRotation);
        //}

        setEyeView(out scv.leftEyeView, eyePosL, eyeRotL);
        setEyeView(out scv.rightEyeView, eyePosR, eyeRotR);

        return scv;
    }

    private void setEyeView(out CameraView view, Vector3 eyePos, Quaternion eyeRot)
    {
        view.eyePos    = convert.toOpenGL(eyePos);
        view.lookAtPos = convert.toOpenGL(eyePos + (eyeRot * Vector3.forward));
        view.camUpDir  = convert.toOpenGL(eyeRot * Vector3.up);
    }
}
