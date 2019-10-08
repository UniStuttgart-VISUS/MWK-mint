using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using interop;

// Returns Camera Parameters as Json string in interop format.
// This script needs to be attached to a Camera Object.
[RequireComponent(typeof(Camera))]
public class CameraConfigurationJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "CameraConfiguration";

    private Camera m_camera = null;

    public void Start()
    {
        m_camera = GetComponent<Camera>();
        
        if (m_camera == null)
            printError();
    }

    private void printError()
    {
        string objectName = gameObject.name;
        Debug.LogError("CameraConfigurationJsonConverter in Object '" + objectName + "' is not attached to a Camera Object!");
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        if (m_camera == null)
        {
            printError();
            return "";
        }

        CameraConfiguration cc = CameraConfigurationFromCamera(m_camera);
        string json = cc.json();
        return json;
	}

    public bool hasChanged()
    {
        return true;
    }

    CameraConfiguration CameraConfigurationFromCamera(Camera cam)
    {
        if(cam == null)
            printError();

        CameraConfiguration cc;

        // CameraView 
        cc.viewParameters.eyePos = convert.toOpenGL(m_camera.transform.position);
        cc.viewParameters.lookAtPos = convert.toOpenGL(m_camera.transform.position + m_camera.transform.forward);
        cc.viewParameters.camUpDir = convert.toOpenGL(m_camera.transform.up);

        // CameraProjection 
        cc.projectionParameters.fieldOfViewY_rad = m_camera.fieldOfView * Mathf.Deg2Rad; // vertical field of view in radians
        cc.projectionParameters.nearClipPlane = m_camera.nearClipPlane;
        cc.projectionParameters.farClipPlane = m_camera.farClipPlane;
        cc.projectionParameters.aspect = m_camera.aspect;
        cc.projectionParameters.pixelWidth = (uint)m_camera.pixelWidth;
        cc.projectionParameters.pixelHeight = (uint)m_camera.pixelHeight;

        // TODO: how to send each eye in stereo!
        cc.viewMatrix = convert.toOpenGL(m_camera.transform.worldToLocalMatrix);
        cc.projectionMatrix = m_camera.projectionMatrix; // TODO: projection already ok for OpenGL?

        return cc;
    }
}
