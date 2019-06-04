using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using interop;

[RequireComponent(typeof(BoundingBoxCornersJsonReceiver))]
public class BBoxCorrectionJsonSender : MonoBehaviour, IJsonStringSendable
{
    public string Name = "BBoxCorrection";
    // Start is called before the first frame update

    private BoundingBoxCornersJsonReceiver m_bboxReceiver = null;
    void Start()
    {
        m_bboxReceiver = gameObject.GetComponent<BoundingBoxCornersJsonReceiver>();
    }

	public string nameString() {
        return this.Name;
	}

	public string jsonString() {
        ModelPose mp = m_bboxReceiver.bboxCorrectionTransform;
        string json = mp.json();
        return json;
	}
	
}
