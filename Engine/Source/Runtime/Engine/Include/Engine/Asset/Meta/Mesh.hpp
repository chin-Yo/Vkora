#pragma once
#include "Meta.hpp"

struct MeshAssetMetadata : public AssetMetadata
{
    MeshAssetMetadata() { type = AssetType::Mesh; }
};
