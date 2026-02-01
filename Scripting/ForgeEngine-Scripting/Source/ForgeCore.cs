using System;
using System.Runtime.CompilerServices;

namespace Forge
{
    public struct Vector3
    {
        public float X, Y, Z;

        public Vector3(float x, float y, float z)
        {
            X = x; Y = y; Z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
    }

    public class Entity
    {
        public readonly uint ID;

        protected Entity() { ID = 0; }
        
        internal Entity(uint id)
        {
            ID = id;
        }

        public string Name
        {
            get => InternalCalls.Entity_GetName(ID);
            set => InternalCalls.Entity_SetName(ID, value);
        }

        public Transform Transform => new Transform(ID);
    }

    public class Transform
    {
        private readonly uint EntityID;

        internal Transform(uint entityID)
        {
            EntityID = entityID;
        }

        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetPosition(EntityID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetPosition(EntityID, ref value);
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.Transform_GetRotation(EntityID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetRotation(EntityID, ref value);
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.Transform_GetScale(EntityID, out Vector3 result);
                return result;
            }
            set => InternalCalls.Transform_SetScale(EntityID, ref value);
        }
    }

    public abstract class MonoBehaviour : Entity
    {
        // These are called by the C++ engine
        protected virtual void OnStart() {}
        protected virtual void OnUpdate(float deltaTime) {}
    }

    internal static class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string Entity_GetName(uint entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Entity_SetName(uint entityID, string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetPosition(uint entityID, out Vector3 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetPosition(uint entityID, ref Vector3 value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRotation(uint entityID, out Vector3 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetRotation(uint entityID, ref Vector3 value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetScale(uint entityID, out Vector3 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetScale(uint entityID, ref Vector3 value);
    }
}
