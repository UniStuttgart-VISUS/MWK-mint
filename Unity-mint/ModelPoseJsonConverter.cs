﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

using interop;

// Returns transform of the GameObject as Json string in interop format.
public class ModelPoseJsonConverter : MonoBehaviour, IJsonStringConvertible {

    public string Name = "ModelPose";

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        ModelPose mc = ModelConfigurationFromTransform(gameObject.transform);
        string json = mc.json();
        return json;
	}
	
    ModelPose ModelConfigurationFromTransform(Transform transform)
    {
        ModelPose mc;
        mc.translation = convert.toOpenGL(transform.localPosition); // TODO: if grabbed by controller, we want position in world space?
        mc.scale = transform.localScale;
        mc.rotation_axis_angle = convert.toOpenGL(transform.rotation);
        mc.modelMatrix = convert.toOpenGL(transform.localToWorldMatrix);

        return mc;
    }
}