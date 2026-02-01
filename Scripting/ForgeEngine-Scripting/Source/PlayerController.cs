using Forge;
using System;

public class PlayerController : MonoBehaviour
{
    public float Speed = 5.0f;

    protected override void OnStart()
    {
        Console.WriteLine($"PlayerController started on Entity: {Name} (ID: {ID})");
        
        // Initial position
        Position = new Vector3(0, 0, 0);
    }

    protected override void OnUpdate(float deltaTime)
    {
        Vector3 pos = Transform.Position;
        
        // Simple movement: move along X axis
        pos.X += Speed * deltaTime;
        
        Transform.Position = pos;

        // Rotate slowly
        Vector3 rot = Transform.Rotation;
        rot.Y += 1.0f * deltaTime;
        Transform.Rotation = rot;
    }
}
