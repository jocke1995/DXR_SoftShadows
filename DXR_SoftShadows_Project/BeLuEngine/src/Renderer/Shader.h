#ifndef SHADER_H
#define SHADER_H

#include <dxcapi.h>

class Shader
{
public:
	Shader(LPCTSTR path, ShaderType type);
	virtual ~Shader();

	IDxcBlob* GetBlob() const;

private:
	IDxcBlob* m_pBlob;
	ShaderType m_Type;
	LPCTSTR m_Path;	// Ex: vertexShader1

	void compileShader();
};

#endif