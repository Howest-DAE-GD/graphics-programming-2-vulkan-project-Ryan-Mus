// Material.cpp
#include "Material.h"

Material::Material()
    : pDiffuseTexture(nullptr), pNormalTexture(nullptr), pMetallicRoughnessTexture(nullptr)
{
    // Constructor implementation (initialize pointers to nullptr or any default initialization)
}

Material::~Material()
{
    // Destructor implementation (clean up resources)
    delete pDiffuseTexture;
    delete pNormalTexture;
    delete pMetallicRoughnessTexture;
}
