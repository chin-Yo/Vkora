#pragma once
#include "Meta.hpp"

struct MeshAssetMetaData : public AssetMetadata
{
    MeshAssetMetaData() { type = AssetType::Mesh; }
};

struct TextureAssetMetaData : public AssetMetadata
{
    TextureAssetMetaData() { type = AssetType::Texture; }
};
