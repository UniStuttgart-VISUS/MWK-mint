using System.Collections;
using System.Collections.Generic;
using UnityEngine; // Vector4, Matrix4x4
using System; // Serializable

// This interop namespace offers the Unity-side implementation of data structures and routines
// to exchange data with an OpenGL application which uses the MWK-mint library to organize its
// data exchange.
namespace interop
{
    using vec4 = Vector4;
    using mat4 = Matrix4x4;

    [Serializable]
    public struct BoundingBoxCorners
    {
        public vec4 min;
        public vec4 max;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<BoundingBoxCorners>(json);  }
    }

    [Serializable]
    public struct ModelPose
    {
        public vec4 translation;
        public vec4 scale;
        public vec4 rotation_axis_angle_rad;

        public mat4 modelMatrix;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<ModelPose>(json);  }
    }

    [Serializable]
    public struct StereoCameraConfiguration
    {
        public float stereoConvergence; // distance to point where left and right eye converge
        public float stereoSeparation; // distance between virtual eyes

        public CameraConfiguration cameraLeftEye;
        public CameraConfiguration cameraRightEye;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<StereoCameraConfiguration>(json);  }
    };

    [Serializable]
    public struct CameraConfiguration
    {
        public CameraView viewParameters;
        public CameraProjection projectionParameters;

        // the matrices hold the respective low-level camera transformations as read from unity
        // the matrices may be from the VR SDK and thus better suited for VR stereo rendering than the explicit camera parameters above
        public mat4 viewMatrix;
        public mat4 projectionMatrix;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<CameraConfiguration>(json);  }
    };

    [Serializable]
    public struct StereoCameraView
    {
        public CameraView leftEyeView;
        public CameraView rightEyeView;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<StereoCameraView>(json);  }
    }

    [Serializable]
    public struct CameraView
    {
        public vec4 eyePos;
        public vec4 lookAtPos;
        public vec4 camUpDir;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<CameraView>(json);  }
    };

    [Serializable]
    public struct CameraProjection
    {
        public float fieldOfViewY_rad; // vertical field of view in radians 
        public float nearClipPlane; // distance of near clipping plane
        public float farClipPlane;

        public float aspect; // aspect ratio, width divided by height
        public uint pixelWidth; // width of camera area in pixels, i.e. framebuffer texture size
        public uint pixelHeight;

        public string json() { return JsonUtility.ToJson(this); }
        public void fromJson(string json) { this = JsonUtility.FromJson<CameraProjection>(json);  }
    };

}
