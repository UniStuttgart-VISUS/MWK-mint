using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

using interop;

public enum DatasetAlignmentMethod {
    viaRenderer,
    viaUnity
}

// Returns transform of the GameObject as Json string in interop format.
public class BoundingBoxCornersJsonReceiver : MonoBehaviour, IJsonStringReceivable {

    public string Name = "BoundingBoxCorners";
    public bool renderBbox = true;
    public DatasetAlignmentMethod imageAlignment = DatasetAlignmentMethod.viaRenderer;
    public bool scaleDatasetDown = false;
    private float m_scaleDownFactor = 1.0f;

    private string m_inJsonString = null;

    private bool m_isBboxSet = false;
    private BoundingBoxCorners m_bbox;
    private Mesh m_bboxMesh = null;
    private Material m_bboxMaterial = null;

    private ModelPose m_bboxCorrectionTransform;

    public ModelPose bboxCorrectionTransform {
        get { return m_bboxCorrectionTransform; }
    }

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

            if(scaleDatasetDown)
                doScaleDatasetDown();
        }

        if(m_bboxMesh && renderBbox)
        {
            Graphics.DrawMesh(m_bboxMesh, this.transform.localToWorldMatrix, m_bboxMaterial, 0);
        }
    }

    void doScaleDatasetDown()
    {
        this.transform.localScale = this.transform.localScale * m_scaleDownFactor;
    }

    private void setBboxRendering(BoundingBoxCorners bbox)
    {
        var bboxMin = new Vector3(bbox.min.x, bbox.min.y, bbox.min.z);
        var bboxMax = new Vector3(bbox.max.x, bbox.max.y, bbox.max.z);

        float minX = bboxMin.x;
        float minY = bboxMin.y;
        float minZ = bboxMin.z;
        float maxX = bboxMax.x;
        float maxY = bboxMax.y;
        float maxZ = bboxMax.z;

        var bboxCenter = bboxMin + (bboxMax - bboxMin)*0.5f;
        var signedDiff = (bboxMax - bboxMin);
        var diff = new Vector3(Mathf.Abs(signedDiff.x), Mathf.Abs(signedDiff.y), Mathf.Abs(signedDiff.z));
        m_scaleDownFactor = 1.0f / Mathf.Max(diff.x, diff.y, diff.z);

        // depending on where the rendered dataset image is aligned, set some offsets
        // two options: (a) the renderer understands model matrices and will receive a matrix to align the dataset to (0,0,0)
        // or (b) we align the dataset used in unity to match the world-space bounding box position we received from the renderer (this slightly breaks dataset rotation)
        Vector3 centering = new Vector3(0.0f, 0.0f, 0.0f);
        switch(imageAlignment) {
            case DatasetAlignmentMethod.viaRenderer:
                centering = -bboxCenter;
                break;
            case DatasetAlignmentMethod.viaUnity:
                centering = new Vector3(0.0f, 0.0f, 0.0f);
                break;
        }
        m_bboxCorrectionTransform.scale = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        m_bboxCorrectionTransform.rotation_axis_angle_rad = new Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        switch(imageAlignment) {
            case DatasetAlignmentMethod.viaRenderer:
                m_bboxCorrectionTransform.translation = convert.toOpenGL(-bboxCenter);
                m_bboxCorrectionTransform.modelMatrix = Matrix4x4.TRS(m_bboxCorrectionTransform.translation, Quaternion.identity, m_bboxCorrectionTransform.scale);
                break;
            case DatasetAlignmentMethod.viaUnity:
                m_bboxCorrectionTransform.translation = new Vector4(0.0f, 0.0f, 0.0f, 0.0f);;
                m_bboxCorrectionTransform.modelMatrix = Matrix4x4.identity;
                break;
        }
        Vector3 meshOffset = new Vector3(0.0f, 0.0f, 0.0f);
        Vector3 colliderOffset = new Vector3(0.0f, 0.0f, 0.0f);
        switch(imageAlignment) {
            case DatasetAlignmentMethod.viaRenderer:
                meshOffset = new Vector3(0.0f, 0.0f, 0.0f);
                colliderOffset = new Vector3(0.0f, 0.0f, 0.0f);
                break;
            case DatasetAlignmentMethod.viaUnity:
                meshOffset = signedDiff*0.5f + bboxMin;
                colliderOffset = bboxCenter;
                break;
        }

        // now set the dataset parameters in unity: bounding box wireframe, resized cube mesh, resized mesh collider
        Vector3[] bboxVertices = new Vector3[] {
            bboxMin                       + centering, // 0
            new Vector3(maxX, minY, minZ) + centering, // 1
            new Vector3(minX, maxY, minZ) + centering, // 2
            new Vector3(maxX, maxY, minZ) + centering, // 3
            new Vector3(minX, minY, maxZ) + centering, // 4
            new Vector3(maxX, minY, maxZ) + centering, // 5
            new Vector3(minX, maxY, maxZ) + centering, // 6
            bboxMax                       + centering, // 7
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
        {
            var newVertex = Vector3.Scale(v, diff) + meshOffset;
            newCubeVertices.Add(newVertex);
        }
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
        thisBoxCollider.center = colliderOffset;
    }
}
