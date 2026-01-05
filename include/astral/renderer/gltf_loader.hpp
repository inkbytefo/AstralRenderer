#pragma once

#include "astral/renderer/model.hpp"
#include <filesystem>

namespace astral {

class Context;
class SceneManager;

class GltfLoader {
public:
    explicit GltfLoader(Context* context);
    ~GltfLoader();

    std::unique_ptr<Model> loadFromFile(const std::filesystem::path& path, SceneManager* sceneManager);

private:
    Context* m_context;
    VkSampler m_defaultSampler;
    std::vector<VkSampler> m_samplers;
    void createDefaultSampler();
};

} // namespace astral
