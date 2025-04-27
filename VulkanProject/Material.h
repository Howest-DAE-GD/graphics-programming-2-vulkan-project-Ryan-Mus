// Material.h
#pragma once
#include <string>
#include <vector>
#include "Texture.h"

class Material {
public:
    Material();
    ~Material();

    // Material properties
    Texture* pDiffuseTexture;
    Texture* pSpecularTexture;
	Texture* pAlphaTexture;
};
