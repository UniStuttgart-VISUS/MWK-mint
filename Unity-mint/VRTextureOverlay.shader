Shader "mint/VRTextureOverlay" {
   Properties {
      _LeftEyeTex ("Left Eye Texture", 2D) = "gray" {}
      _RightEyeTex ("Right Eye Texture", 2D) = "bump" {}
      _LeftTextureMappingUVs ("Left Texture UVs", Vector) = (0.0, 1.0, 0.0, 1.0) // uMin, uMax, vMin, vMax
      _RightTextureMappingUVs ("Right Texture UVs", Vector) = (0.0, 1.0, 0.0, 1.0) // uMin, uMax, vMin, vMax
      _OverlayColor ("OverlayColor", Color) = (0.0, 0.0, 0.0, 0.0)
      //[IntRange] _StencilRef ("Stencil Reference Value", Range(0,255)) = 0
   }

   SubShader {
      Tags { "Queue" = "Overlay" } // render after everything else
      LOD 100
 
      Pass {
         Blend SrcAlpha OneMinusSrcAlpha // use alpha blending
         Cull Off // no backface culling of dataset box => we can look also into dataset
         //ZTest Always // deactivate depth test
 
        //Stencil{
        //    Ref [_StencilRef]
        //    comp Equal
        //    Pass keep
        // }
 
         CGPROGRAM
 
         #pragma vertex vert  
         #pragma fragment frag
         #pragma target 3.0
         //#pragma multi_compile __ STEREO_RENDER
         #include "UnityCG.cginc" // defines float4 _ScreenParams with x = screen width; y = screen height;
 
         // User-specified uniforms
         uniform sampler2D _LeftEyeTex;
         uniform sampler2D _RightEyeTex;
         uniform float4 _OverlayColor;
         uniform float4 _LeftTextureMappingUVs;
         uniform float4 _RightTextureMappingUVs;
 
         struct vertexInput {
            float4 vertex : POSITION;
            float4 texcoord : TEXCOORD0;
         };
         struct vertexOutput {
            //float4 pos : SV_POSITION; // collides with VPOS
            float4 tex : TEXCOORD0;
            
         };
 
         vertexOutput vert(vertexInput input, out float4 pos : SV_POSITION) 
         {
            vertexOutput output;

            pos = UnityObjectToClipPos(input.vertex);
            output.tex = input.texcoord;
 
            return output;
         }
 
         float4 frag(vertexOutput input, UNITY_VPOS_TYPE screenPos : VPOS) : SV_Target
         {
            // code adapted from: https://forum.unity.com/threads/solved-right-eye-rendering-issues-for-vr-portal-mirror-effect.569125/
            float2 screenUV = screenPos.xy / _ScreenParams.xy;
            float4 color = float4(0.0, 0.0, 0.0, 0.0);

            if (unity_CameraProjection[0][2] < 0.0) {
               float2 texCoord = lerp(_LeftTextureMappingUVs.xz, _LeftTextureMappingUVs.yw, screenUV.xy);
               color = tex2D(_LeftEyeTex, texCoord.xy);
            } else {
               float2 texCoord = lerp(_RightTextureMappingUVs.xz, _RightTextureMappingUVs.yw, screenUV.xy);
               color = tex2D(_RightEyeTex, texCoord);
            }

            return _OverlayColor + color;
         }
 
         ENDCG
      }
   }
}