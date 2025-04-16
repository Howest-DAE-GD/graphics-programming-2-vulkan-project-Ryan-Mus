class InstanceBuilder 
{
public:
    InstanceBuilder& setApplicationInfo(const VkApplicationInfo& appInfo) 
    {
        m_ApplicationInfo = appInfo;
        return *this;
    }

    InstanceBuilder& enableExtensions(const std::vector<const char*>& extensions) 
    {
        m_EnabledExtensions = extensions;
        return *this;
    }

    InstanceBuilder& enableValidationLayers(const std::vector<const char*>& layers) 
    {
        m_ValidationLayers = layers;
        return *this;
    }

    InstanceBuilder& setDebugMessengerCreateInfo(const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) 
    {
        this->m_DebugCreateInfo = debugCreateInfo;
        return *this;
    }

    VkResult build(VkInstance& instance) const 
    {
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &m_ApplicationInfo;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
        createInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        if (!m_ValidationLayers.empty()) 
        {
            createInfo.pNext = &m_DebugCreateInfo;
        }
        else 
        {
            createInfo.pNext = nullptr;
        }

        return vkCreateInstance(&createInfo, nullptr, &instance);
    }

private:
    VkApplicationInfo m_ApplicationInfo{};
    std::vector<const char*> m_EnabledExtensions{};
    std::vector<const char*> m_ValidationLayers{};
    VkDebugUtilsMessengerCreateInfoEXT m_DebugCreateInfo{};
};
