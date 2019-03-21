using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

using interop;

// Returns transform of the GameObject as Json string in interop format.
public class BoundingBoxCornersJsonReceiver : MonoBehaviour, IJsonStringReceivable {

    public string Name = "BoundingBoxCorners";

    private string m_inJsonString = null;
    private BoundingBoxCorners m_bbox;

	public string nameString() {
        return this.Name;
	}

	public void setJsonString(string json) {
        m_inJsonString = json;
	}
	
    BoundingBoxCorners BoundingBoxCornersFromString()
    {
        BoundingBoxCorners bb = new BoundingBoxCorners();
        bb.fromJson(m_inJsonString);

        bb.min = convert.toUnity(bb.min);
        bb.max = convert.toUnity(bb.max);

        return bb;
    }
}
