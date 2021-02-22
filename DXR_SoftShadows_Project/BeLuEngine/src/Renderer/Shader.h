#ifndef SHADER_H
#define SHADER_H

#include <dxcapi.h>

class Shader
{
public:
	Shader(LPCTSTR path, SHADER_TYPE type);
	virtual ~Shader();

	IDxcBlob* GetBlob() const;

private:
	IDxcBlob* m_pBlob;
	SHADER_TYPE m_Type;
	LPCTSTR m_Path;	// Ex: vertexShader1

	void compileShader();
};

#endif