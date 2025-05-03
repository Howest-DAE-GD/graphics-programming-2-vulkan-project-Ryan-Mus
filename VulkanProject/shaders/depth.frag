#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D diffuseSampler;

void main() 
{
   if(texture(diffuseSampler, fragTexCoord).a < 0.5) 
   {
	  discard; // Discard fragments with low alpha
   }
}
