using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

using interop;

// Returns transform of the GameObject as Json string in interop format.
public class BoundingBoxCornersJsonReceiver : MonoBehaviour, IJsonStringReceivable {

    public string Name = "BoundingBoxCorners";
    public bool renderBbox = true;

    private string m_inJsonString = null;

    private bool m_isBboxSet = false;
    private BoundingBoxCorners m_bbox;
    private Mesh m_bboxMesh = null;
    private Material m_bboxMaterial = null;

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

        m_inJsonString = null;

        return bb;
    }

    void Update()
    {
        if (m_inJsonString != null && !m_isBboxSet)
        {
            m_bbox = BoundingBoxCornersFromString();
            setBboxRendering(m_bbox);
            m_isBboxSet = true;
        }

        if(m_bboxMesh && renderBbox)
        {
            Graphics.DrawMesh(m_bboxMesh, this.transform.localToWorldMatrix, m_bboxMaterial, 0);
        }
    }
    private void setBboxRendering(BoundingBoxCorners bbox)
    {
        var bboxMin = convert.toUnity(new Vector3(bbox.min.x, bbox.min.y, bbox.min.z));
        var bboxMax = convert.toUnity(new Vector3(bbox.max.x, bbox.max.y, bbox.max.z));

        float minX = bboxMin.x;
        float minY = bboxMin.y;
        float minZ = bboxMin.z;
        float maxX = bboxMax.x;
        float maxY = bboxMax.y;
        float maxZ = bboxMax.z;

        var bboxCenter = bboxMin + (bboxMax - bboxMin)*0.5f;
        var diff = (bboxMax - bboxMin);

        var scaleDownTo1 = Vector3.Scale(this.transform.localScale, new Vector3(1.0f / diff.x, 1.0f / diff.y, 1.0f / diff.z));
        this.transform.localScale = scaleDownTo1;

        // center of bbox is set to (0,0,0)
        Vector3[] bboxVertices = new Vector3[] {
            bboxMin                       - bboxCenter, // 0
            new Vector3(maxX, minY, minZ) - bboxCenter, // 1
            new Vector3(minX, maxY, minZ) - bboxCenter, // 2
            new Vector3(maxX, maxY, minZ) - bboxCenter, // 3
            new Vector3(minX, minY, maxZ) - bboxCenter, // 4
            new Vector3(maxX, minY, maxZ) - bboxCenter, // 5
            new Vector3(minX, maxY, maxZ) - bboxCenter, // 6
            bboxMax                       - bboxCenter, // 7
        };

        // connect wire frame
        int[] indices = new int[] {
            0, 1,
            0, 2,
            1, 3,
            2, 3,

            0, 4,
            1, 5,
            3, 7,
            2, 6,

            4, 5,
            4, 6,
            5, 7,
            6, 7
        };
        // set up bbox wire rendering
        m_bboxMesh = new Mesh();
        m_bboxMesh.vertices = bboxVertices;
        m_bboxMesh.SetIndices(indices, MeshTopology.Lines, 0);
        m_bboxMaterial = new Material(Shader.Find("Unlit/Color"));
        m_bboxMaterial.color = new Color(0.8f, 0.8f, 0.0f); // rgb

        // resize cube mesh to size of dataset bounding box
        var thisCubeMesh = this.GetComponent<MeshFilter>();
        List<Vector3> cubeVertices = new List<Vector3>();
        List<Vector3> newCubeVertices = new List<Vector3>();
        thisCubeMesh.mesh.GetVertices(cubeVertices);
        foreach(var v in cubeVertices)
            newCubeVertices.Add(Vector3.Scale(v, diff));
        thisCubeMesh.mesh.SetVertices(newCubeVertices);

        // set up box collider for controller interaction and so on
        var thisBoxCollider = this.GetComponent<BoxCollider>();
        if (thisBoxCollider == null)
        {
            this.gameObject.AddComponent<BoxCollider>();
            thisBoxCollider = this.GetComponent<BoxCollider>();
        }
        thisBoxCollider.enabled = true;
        thisBoxCollider.size = diff;
    }
}
