using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using interop;

// Returns transform of the GameObject as Json string in interop format.
public class ModelPoseJsonSender : MonoBehaviour, IJsonStringSendable {

    public string Name = "ModelPose";
    private Transform lastSentTransform = new RectTransform();

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        ModelPose mc = ModelConfigurationFromTransform(gameObject.transform);
        lastSentTransform = gameObject.transform;
        string json = mc.json();
        return json;
	}

	public bool hasChanged()
	{
		return lastSentTransform != gameObject.transform;
	}

	ModelPose ModelConfigurationFromTransform(Transform transform)
    {
        ModelPose mc;
        mc.translation = convert.toOpenGL(transform.localPosition); // TODO: if grabbed by controller, we want position in world space?
        mc.scale = transform.localScale;
        mc.rotation_axis_angle_rad = convert.toOpenGL(transform.rotation);
        mc.modelMatrix = convert.toOpenGL(transform.localToWorldMatrix);

        return mc;
    }
}
