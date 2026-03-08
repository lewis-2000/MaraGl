#pragma once

#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <json.hpp>

class Model;

namespace MaraGl
{
    class Scene;

    enum class AssetType
    {
        Model,
        Scene,
        Texture,
        Skybox
    };

    struct LoadRequest
    {
        AssetType type;
        std::string path;
        std::string name;
        std::function<void(bool success, const std::string &errorMsg)> callback;
        void *userData = nullptr;
    };

    struct LoadingProgress
    {
        std::string assetName;
        std::string assetPath;
        AssetType type;
        float progress; // 0.0 to 1.0
        bool isComplete;
        bool success;
        std::string errorMessage;
    };

    class AssetLoader
    {
    public:
        AssetLoader();
        ~AssetLoader();

        // Start async loading of a model
        void LoadModelAsync(const std::string &path, const std::string &name,
                            std::function<void(std::shared_ptr<Model>, bool success, const std::string &error)> callback);

        // Start async loading of a scene
        void LoadSceneAsync(Scene *scene, const std::string &path,
                            std::function<void(bool success, const std::string &error)> callback);

        // Start async loading of a skybox
        void LoadSkyboxAsync(Scene *scene, const std::string &path,
                             std::function<void(bool success, const std::string &error)> callback);

        // Check if any assets are currently loading
        bool IsLoading() const { return m_ActiveLoads > 0; }

        // Get current loading progress
        std::vector<LoadingProgress> GetLoadingProgress();

        // Process completed loads on main thread (call this from main thread)
        void ProcessCompletedLoads();

        // Cancel all pending loads
        void CancelAll();

    private:
        void WorkerThread();
        void ProcessModelLoad(const LoadRequest &request);
        void ProcessSceneLoad(const LoadRequest &request);
        void ProcessSkyboxLoad(const LoadRequest &request);

        std::queue<LoadRequest> m_RequestQueue;
        std::mutex m_QueueMutex;
        std::condition_variable m_QueueCV;
        std::atomic<bool> m_Running;
        std::vector<std::thread> m_Workers;
        std::atomic<int> m_ActiveLoads;

        // Current loading progress
        std::vector<LoadingProgress> m_CurrentProgress;
        std::mutex m_ProgressMutex;

        // Completed loads to process on main thread
        struct CompletedLoad
        {
            AssetType type = AssetType::Model;
            std::shared_ptr<Model> model;
            std::function<void(std::shared_ptr<Model>, bool, const std::string &)> modelCallback;
            std::function<void(bool, const std::string &)> genericCallback;
            Scene *scene = nullptr;
            std::string path;
            nlohmann::json sceneJson;
            std::unordered_map<std::string, std::shared_ptr<Model>> preloadedModels;
            bool success;
            std::string errorMessage;
            bool isModelLoad;
        };
        std::queue<CompletedLoad> m_CompletedLoads;
        std::mutex m_CompletedMutex;
    };
}
