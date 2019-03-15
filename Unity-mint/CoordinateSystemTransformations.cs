using System.Collections;
using System.Collections.Generic;
using UnityEngine; // Vector4, Matrix4, Quaternion, Vector3

// Here we have a load of convenience-methods to convert data types
// between Left-Hand-Side (lhs, Unity) and Right-Hand-Side (rhs, OpenGL)
// coordinate systems.
namespace interop
{
    using vec4 = Vector4;
    using vec3 = Vector3;
    using mat4 = Matrix4x4;

    // OOP is stupid, so we need a class to wrap otherwise perfectly fine free functions
    public class convert
    {
        public static vec4 lhs_to_rhs(vec4 lhs)
        {
            return new vec4(lhs.x, lhs.y, -lhs.z, lhs.w);
        }

        public static vec3 lhs_to_rhs(vec3 lhs)
        {
            return new vec3(lhs.x, lhs.y, -lhs.z);
        }

        public static vec4 rhs_to_lhs(vec4 rhs)
        {
            return lhs_to_rhs(rhs); // is symmetric
        }

        public static vec3 rhs_to_lhs(vec3 rhs)
        {
            return lhs_to_rhs(rhs); // is symmetric
        }

        public static vec4 toOpenGL(vec4 unity)
        {
            return lhs_to_rhs(unity);
        }

        public static vec3 toOpenGL(vec3 unity)
        {
            return lhs_to_rhs(unity);
        }

        public static vec4 toUnity(vec4 ogl)
        {
            return rhs_to_lhs(ogl);
        }

        public static vec3 toUnity(vec3 ogl)
        {
            return rhs_to_lhs(ogl);
        }

        // lhs transform matrix (TRS: translate, rotate, scale) to rhs: revert z axis
        public static mat4 lhs_to_rhs(mat4 lhs)
        {
            var m = mat4.identity;
            m.m22 = -1.0f;
            return m * lhs * m.inverse; // matrix base change given by z-axis-mirroring transform
        }
        public static mat4 rhs_to_lhs(mat4 rhs)
        {
            return lhs_to_rhs(rhs);
        }

        public static mat4 toOpenGL(mat4 unity)
        {
            return lhs_to_rhs(unity);
        }
        public static mat4 toUnity(mat4 ogl)
        {
            return rhs_to_lhs(ogl);
        }

        // pass quaternion rotation as (axis, angle) = (x,y,z,w)
        public static vec4 toOpenGL(Quaternion unity)
        {
            float angle_degree = 0.0f;
            Vector3 axis = Vector3.zero;
            unity.ToAngleAxis(out angle_degree, out axis);
            float angle_radians = Mathf.Deg2Rad * angle_degree; // GLM expects radians on OpenGL side

            return new Vector4(
                axis.x,
                axis.y,
                -axis.z, // lhs axis+angle => rhs axis+angle
                -angle_radians);
            }
    }

}
