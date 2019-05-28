using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Klak.Spout;
using System.Linq; // functional ops


// Searches for two cameras in the scene, called 'CameraLeft' and 'CameraRight',
// and attaches textured quads to them, which show textures received via Spout
public class SpoutStereoPlanes : MonoBehaviour
{
    public string sourceName = "/UnityInterop/";
    public string vrCamerasBaseName = "Camera";

    EyeResources[] stereoResources = null;

    // Start is called before the first frame update
    void Start()
    {
        stereoResources = new EyeResources[2] {new EyeResources(), new EyeResources()}; // ARGH

        stereoResources[(int)stereo.Left].init(stereo.Left, vrCamerasBaseName, sourceName);
        stereoResources[(int)stereo.Right].init(stereo.Right, vrCamerasBaseName, sourceName);
    }

    // Update is called once per frame
    void Update()
    {
        foreach(var sr in this.stereoResources)
            sr.update(); // resize texture if camera resolution changed
    }
}


// Resources needed to render a transparent plane in front of camera
class EyeResources {
    public stereo side;
    public Camera camera;
    public string sourceName;
    public SpoutReceiver spout;
    public RenderTexture texture;
    public Material material;
    public GameObject meshHolder;
    public Mesh quadMesh;
    public MeshRenderer meshRenderer;

    public void init(stereo side, string cameraBaseName, string interopBaseName)
    {
        this.side = side;
        string cameraSuffix = (side == stereo.Left) ? ("Left") : ("Right");
        string cameraName = cameraBaseName + cameraSuffix;

        var cameras = GameObject.FindObjectsOfType<Camera>();
        foreach (var c in cameras)
            if (c.name == cameraName)
                this.camera = c; // TODO: not safe

        if (this.camera == null)
        {
            Debug.LogError("EyeResources init(): could not find camera named '" + cameraName + "'");
            return;
        }
        this.meshHolder = new GameObject(cameraName + "Mesh");

        this.sourceName = interopBaseName + cameraName;
        this.meshHolder.AddComponent<SpoutReceiver>(); // need to attach SpoutReceiver Script to some object, can not simply make 'new'
        this.spout = this.meshHolder.GetComponent<SpoutReceiver>();
        spout.sourceName = this.sourceName;
        spout.targetTexture = this.texture;

        // set up colored quad with spout texture in front of camera
        this.material = new Material(Shader.Find("Unlit/Transparent"));
        this.material.name = cameraName + "Material";
        this.setTexture(cameraName + "Texture", 1, 1);

        // we have to use a GameObject to make the quad follow the cameras position
        this.meshHolder.AddComponent<MeshFilter>();
        this.meshHolder.AddComponent<MeshRenderer>();
        this.quadMesh = this.meshHolder.GetComponent<MeshFilter>().mesh;
        this.meshHolder.transform.SetParent(this.camera.transform, false);
        this.meshRenderer = this.meshHolder.GetComponent<MeshRenderer>();
        this.meshRenderer.materials = new Material[] { this.material };

        var eye = (side == stereo.Left) ? (Camera.MonoOrStereoscopicEye.Left) : (Camera.MonoOrStereoscopicEye.Right);
        // if camera is in mono mode (e.g. for testing), the frustum corners calculated for Left/Right eye frustum are broken
        if (camera.stereoActiveEye == Camera.MonoOrStereoscopicEye.Mono)
            eye = Camera.MonoOrStereoscopicEye.Mono;

        setQuadToCameraCorners(this.quadMesh, this.camera, eye);
    }

    private void setTexture(string name, int width, int height)
    {
        if(this.texture != null)
        {
            spout.targetTexture = null;
            material.mainTexture = null;
            this.texture.Release();
            this.texture = null;
        }

        var newTex = new RenderTexture(width, height, 1);
        newTex.name = name;

        this.texture = newTex;
        spout.targetTexture = newTex;
        material.mainTexture = newTex;
    }

    void setQuadToCameraCorners(Mesh mesh, Camera cam, Camera.MonoOrStereoscopicEye eye)
    {
        Vector3[] nearPlaneCorners = new Vector3[4]; // camera returns corners in view space

        // NOTE: attaching the render plane at nearClipPlane distance leads to mis-alignment of bounding boxes and thus incorret stereo rendering.
        // this is probably due to the non-skewed projection matrices used for rendering in MegaMol.
        // until we can use the skewed VR-Projection-Matrices, use a small offset when setting the render plane.
        cam.CalculateFrustumCorners(new Rect(0,0,1,1), cam.nearClipPlane + 0.32f, eye, nearPlaneCorners);
        mesh.vertices = nearPlaneCorners;

        Vector2[] uvCoords = new Vector2[4] {new Vector2(0.0f,0.0f), new Vector2(0.0f,1.0f), new Vector2(1.0f,1.0f), new Vector2(1.0f,0.0f)};
        mesh.uv = uvCoords;

        int[] indices = new int[6] { 0, 1, 2 , 0, 2, 3 };
        mesh.SetIndices(indices, MeshTopology.Triangles, 0);

        // fixes disappearing of mesh when looked at from certain angles
        mesh.RecalculateNormals();
        mesh.RecalculateBounds();
        mesh.RecalculateTangents();
    }

    public void update()
    {
        if (this.camera == null)
            return;

        if(texture.width != camera.pixelWidth || texture.height != camera.pixelHeight)
        {
            setTexture(string.Copy(texture.name), camera.pixelWidth, camera.pixelHeight);
        }
    }
}

enum stereo
{
    Left = 0,
    Right = 1,
}
	
