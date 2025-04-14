class InstanceBuilder {
public:
    InstanceBuilder& setApplicationInfo(const VkApplicationInfo& appInfo) {
        applicationInfo = appInfo;
        return *this;
    }

    InstanceBuilder& enableExtensions(const std::vector<const char*>& extensions) {
        enabledExtensions = extensions;
        return *this;
    }

    InstanceBuilder& enableValidationLayers(const std::vector<const char*>& layers) {
        validationLayers = layers;
        return *this;
    }

    InstanceBuilder& setDebugMessengerCreateInfo(const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) {
        this->debugCreateInfo = debugCreateInfo;
        return *this;
    }

    VkResult build(VkInstance& instance) const {
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &applicationInfo;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        if (!validationLayers.empty()) {
            createInfo.pNext = &debugCreateInfo;
        }
        else {
            createInfo.pNext = nullptr;
        }

        return vkCreateInstance(&createInfo, nullptr, &instance);
    }

private:
    VkApplicationInfo applicationInfo{};
    std::vector<const char*> enabledExtensions{};
    std::vector<const char*> validationLayers{};
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
};
