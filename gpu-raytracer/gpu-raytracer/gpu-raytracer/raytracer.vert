varying vec2 position;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	position = gl_Position.xy;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}