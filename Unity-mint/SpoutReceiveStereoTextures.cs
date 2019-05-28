using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Klak.Spout;
using System.Linq; // functional ops
using Valve.VR;

// Searches for two cameras in the scene, called 'CameraLeft' and 'CameraRight',
// and attaches textured quads to them, which show textures received via Spout
public class SpoutReceiveStereoTextures: MonoBehaviour
{
    public string interopBaseName = "/UnityInterop/";
    public string instanceName = "DefaultName"; // gets 'Left' and 'Right' attached for left and right texture

    private EyeResources[] stereoResources = null;

    public Vector2 leftTextureSize = new Vector2(0.0f, 0.0f);
    public Vector2 rightTextureSize = new Vector2(0.0f, 0.0f);
    private Material renderVROverlayMaterial = null;

    // Start is called before the first frame update
    void Start()
    {
        stereoResources = new EyeResources[2] {new EyeResources(), new EyeResources()};

        stereoResources[(int)stereo.Left].init(stereo.Left, interopBaseName, instanceName, this.gameObject);
        stereoResources[(int)stereo.Right].init(stereo.Right, interopBaseName, instanceName, this.gameObject);
    }

    // Update is called once per frame
    void Update()
    {
        // spout initializes textures lazily during update
        // so we need to do the same
        // we also need to check for size-changes of the texture

        var left = stereoResources[(int)stereo.Left];
        var right = stereoResources[(int)stereo.Right];

        if(left.updateTexture() || right.updateTexture())
        {
            var leftTex = left.targetTexture;
            var rightTex = right.targetTexture;

            leftTextureSize = new Vector2((float)leftTex.width, (float)leftTex.height);
            rightTextureSize = new Vector2((float)rightTex.width, (float)rightTex.height);

            renderVROverlayMaterial = new Material(Shader.Find("mint/VRTextureOverlay"));
            renderVROverlayMaterial.SetTexture("_LeftEyeTex", leftTex);
            renderVROverlayMaterial.SetTexture("_RightEyeTex", rightTex);

            //if(OpenVR.IsHmdPresent())
            {
                var tbL = SteamVR.instance.textureBounds[(int)stereo.Left];
                var tbR = SteamVR.instance.textureBounds[(int)stereo.Right];

                renderVROverlayMaterial.SetVector("_LeftTextureMappingUVs", new Vector4(tbL.uMin, tbL.uMax, tbL.vMin, tbL.vMax));// uMin, uMax, vMin, vMax
                renderVROverlayMaterial.SetVector("_RightTextureMappingUVs", new Vector4(tbR.uMin, tbR.uMax, tbR.vMin, tbR.vMax));

                Debug.Log("LeftEye TextureBounds: " + tbL.uMin +", "+ tbL.uMax+", "+ tbL.vMin+", "+ tbL.vMax);
                Debug.Log("RightEye TextureBounds: " + tbR.uMin +", "+ tbR.uMax+", "+ tbR.vMin+", "+ tbR.vMax);
            }

            var meshRenderer = this.gameObject.GetComponent<MeshRenderer>();
            meshRenderer.materials = new Material[] { this.renderVROverlayMaterial };
        }
    }


// Resources needed to render a transparent plane in front of camera
    class EyeResources {
        public stereo side;
        public string spoutName;
        public SpoutReceiver spout;

        public RenderTexture targetTexture = null;

        public void init(stereo side, string baseName, string instanceName, GameObject go)
        {
            this.side = side;
            string textureSuffix = (side == stereo.Left) ? ("Left") : ("Right");
            this.spoutName = baseName + instanceName + textureSuffix;

            this.spout = go.AddComponent<SpoutReceiver>() as SpoutReceiver;
            spout.sourceName = this.spoutName;

            setTexture(1, 1);
        }

        public bool updateTexture()
        {
            if(spout.textureChanged)
            {
                setTexture(spout.texWidth, spout.texHeight);
                return true;
            }
            return false;
        }

        private void setTexture(uint width, uint height)
        {
            targetTexture = new RenderTexture((int)width, (int)height, 1);
            spout.targetTexture = targetTexture;
        }
    }

    enum stereo
    {
        Left = 0,
        Right = 1,
    }

}