#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 3) uniform sampler2D alphaSampler;

void main() 
{
   if(texture(alphaSampler, fragTexCoord).r < 0.5) 
   {
	  discard; // Discard fragments with low alpha
   }
}
