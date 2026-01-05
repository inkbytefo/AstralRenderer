#pragma once

#include "astral/core/context.hpp"
#include "astral/resources/image.hpp"
#include "astral/renderer/compute_pipeline.hpp"
#include <memory>
#include <string>

namespace astral {

class EnvironmentManager {
public:
    EnvironmentManager(Context* context);
    ~EnvironmentManager();

    void loadHDR(const std::string& path);
    
    uint32_t getSkyboxIndex() const { return m_skyboxIndex; }
    uint32_t getIrradianceIndex() const { return m_irradianceIndex; }
    uint32_t getPrefilteredIndex() const { return m_prefilteredIndex; }
    uint32_t getBrdfLutIndex() const { return m_brdfLutIndex; }

private:
    Context* m_context;
    
    std::unique_ptr<Image> m_skybox;
    std::unique_ptr<Image> m_irradiance;
    std::unique_ptr<Image> m_prefiltered;
    std::unique_ptr<Image> m_brdfLut;

    uint32_t m_skyboxIndex = (uint32_t)-1;
    uint32_t m_irradianceIndex = (uint32_t)-1;
    uint32_t m_prefilteredIndex = (uint32_t)-1;
    uint32_t m_brdfLutIndex = (uint32_t)-1;

    void convertEquirectToCube(const std::unique_ptr<Image>& equirect);
    void generateIrradiance();
    void generatePrefiltered();
    void generateBrdfLut();
};

} // namespace astral
