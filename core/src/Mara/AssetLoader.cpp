#include "include/AssetLoader.h"
#include "Model.h"
#include "Scene.h"
#include "SceneSerializer.h"
#include "MeshComponent.h"
#include <iostream>
#include <exception>
#include <filesystem>
#include <unordered_set>

namespace MaraGl
{
    AssetLoader::AssetLoader()
        : m_Running(true), m_ActiveLoads(0)
    {
        // Create worker threads (use 2-4 threads for asset loading)
        unsigned int numThreads = std::min(4u, std::thread::hardware_concurrency());
        if (numThreads == 0)
            numThreads = 2;

        // std::cout << "[AssetLoader] Starting " << numThreads << " worker threads" << std::endl;
        for (unsigned int i = 0; i < numThreads; ++i)
        {
            m_Workers.emplace_back(&AssetLoader::WorkerThread, this);
        }
    }

    AssetLoader::~AssetLoader()
    {
        // std::cout << "[AssetLoader] Shutting down..." << std::endl;
        m_Running = false;
        m_QueueCV.notify_all();

        for (auto &worker : m_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        // std::cout << "[AssetLoader] Shutdown complete" << std::endl;
    }

    void AssetLoader::LoadModelAsync(const std::string &path, const std::string &name,
                                     std::function<void(std::shared_ptr<Model>, bool, const std::string &)> callback)
    {
        LoadRequest request;
        request.type = AssetType::Model;
        request.path = path;
        request.name = name;
        request.userData = new std::function<void(std::shared_ptr<Model>, bool, const std::string &)>(callback);

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_RequestQueue.push(request);
        }
        m_QueueCV.notify_one();

        // std::cout << "[AssetLoader] Queued model load: " << path << std::endl;
    }

    void AssetLoader::LoadSceneAsync(Scene *scene, const std::string &path,
                                     std::function<void(bool, const std::string &)> callback)
    {
        LoadRequest request;
        request.type = AssetType::Scene;
        request.path = path;
        request.name = path;
        request.callback = callback;
        request.userData = scene;

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_RequestQueue.push(request);
        }
        m_QueueCV.notify_one();

        // std::cout << "[AssetLoader] Queued scene load: " << path << std::endl;
    }

    void AssetLoader::LoadSkyboxAsync(Scene *scene, const std::string &path,
                                      std::function<void(bool, const std::string &)> callback)
    {
        LoadRequest request;
        request.type = AssetType::Skybox;
        request.path = path;
        request.name = path;
        request.callback = callback;
        request.userData = scene;

        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            m_RequestQueue.push(request);
        }
        m_QueueCV.notify_one();

        // std::cout << "[AssetLoader] Queued skybox load: " << path << std::endl;
    }

    std::vector<LoadingProgress> AssetLoader::GetLoadingProgress()
    {
        std::lock_guard<std::mutex> lock(m_ProgressMutex);
        return m_CurrentProgress;
    }

    void AssetLoader::ProcessCompletedLoads()
    {
        while (true)
        {
            CompletedLoad load;
            {
                std::lock_guard<std::mutex> lock(m_CompletedMutex);
                if (m_CompletedLoads.empty())
                    break;
                load = std::move(m_CompletedLoads.front());
                m_CompletedLoads.pop();
            }

            if (load.type == AssetType::Model && load.modelCallback)
            {
                load.modelCallback(load.model, load.success, load.errorMessage);
            }
            else if (load.type == AssetType::Scene && load.genericCallback)
            {
                bool success = false;
                std::string error;
                if (!load.scene)
                {
                    error = "Invalid scene pointer";
                }
                else
                {
                    try
                    {
                        load.scene->ClearScene();
                        success = SceneSerializer::ApplySceneData(load.scene, load.sceneJson, false);

                        if (success)
                        {
                            // Attach preloaded models quickly (no heavy disk parse here).
                            for (const auto &entity : load.scene->GetEntities())
                            {
                                auto *meshComp = entity->GetComponent<MeshComponent>();
                                if (!meshComp || meshComp->ModelPath.empty())
                                    continue;

                                auto it = load.preloadedModels.find(meshComp->ModelPath);
                                if (it != load.preloadedModels.end())
                                    meshComp->ModelPtr = it->second;
                            }
                        }
                        else
                        {
                            error = "Failed to apply scene data";
                        }
                    }
                    catch (const std::exception &e)
                    {
                        error = std::string("Failed to load scene: ") + e.what();
                    }
                    catch (...)
                    {
                        error = "Unknown error loading scene";
                    }
                }
                load.genericCallback(success, error);
            }
            else if (load.type == AssetType::Skybox && load.genericCallback)
            {
                bool success = false;
                std::string error;
                if (!load.scene)
                {
                    error = "Invalid scene pointer";
                }
                else
                {
                    try
                    {
                        success = load.scene->LoadSkybox(load.path);
                        if (!success)
                            error = "Failed to load skybox from file";
                    }
                    catch (const std::exception &e)
                    {
                        error = std::string("Failed to load skybox: ") + e.what();
                    }
                    catch (...)
                    {
                        error = "Unknown error loading skybox";
                    }
                }
                load.genericCallback(success, error);
            }
            else if (load.genericCallback)
            {
                load.genericCallback(load.success, load.errorMessage);
            }
        }
    }

    void AssetLoader::CancelAll()
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        while (!m_RequestQueue.empty())
        {
            m_RequestQueue.pop();
        }
        // std::cout << "[AssetLoader] Cancelled all pending loads" << std::endl;
    }

    void AssetLoader::WorkerThread()
    {
        while (m_Running)
        {
            LoadRequest request;

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_QueueCV.wait(lock, [this]
                               { return !m_RequestQueue.empty() || !m_Running; });

                if (!m_Running)
                    break;

                if (m_RequestQueue.empty())
                    continue;

                request = m_RequestQueue.front();
                m_RequestQueue.pop();
            }

            m_ActiveLoads++;

            // Add to progress tracking
            {
                std::lock_guard<std::mutex> lock(m_ProgressMutex);
                LoadingProgress progress;
                progress.assetName = request.name;
                progress.assetPath = request.path;
                progress.type = request.type;
                progress.progress = 0.0f;
                progress.isComplete = false;
                progress.success = false;
                m_CurrentProgress.push_back(progress);
            }

            // Process the request based on type
            switch (request.type)
            {
            case AssetType::Model:
                ProcessModelLoad(request);
                break;
            case AssetType::Scene:
                ProcessSceneLoad(request);
                break;
            case AssetType::Skybox:
                ProcessSkyboxLoad(request);
                break;
            default:
                std::cerr << "[AssetLoader] Unknown asset type" << std::endl;
                break;
            }

            // Remove from progress tracking
            {
                std::lock_guard<std::mutex> lock(m_ProgressMutex);
                m_CurrentProgress.erase(
                    std::remove_if(m_CurrentProgress.begin(), m_CurrentProgress.end(),
                                   [&request](const LoadingProgress &p)
                                   {
                                       return p.assetPath == request.path;
                                   }),
                    m_CurrentProgress.end());
            }

            m_ActiveLoads--;
        }
    }

    void AssetLoader::ProcessModelLoad(const LoadRequest &request)
    {
        CompletedLoad completed;
        completed.type = AssetType::Model;
        completed.isModelLoad = true;
        completed.success = false;
        completed.model = nullptr;
        completed.errorMessage = "";

        // Extract callback from userData
        // std::cout << "[AssetLoader] ProcessModelLoad: userData=" << request.userData << std::endl;

        auto *callbackPtr = static_cast<std::function<void(std::shared_ptr<Model>, bool, const std::string &)> *>(request.userData);
        if (callbackPtr)
        {
            // std::cout << "[AssetLoader] ProcessModelLoad: Callback extracted from userData" << std::endl;
            completed.modelCallback = *callbackPtr;
            delete callbackPtr;
        }
        else
        {
            // std::cout << "[AssetLoader] ProcessModelLoad: ERROR - callbackPtr is null!" << std::endl;
        }

        try
        {
            // std::cout << "[AssetLoader] Loading model: " << request.path << std::endl;

            // Update progress
            {
                std::lock_guard<std::mutex> lock(m_ProgressMutex);
                for (auto &progress : m_CurrentProgress)
                {
                    if (progress.assetPath == request.path)
                    {
                        progress.progress = 0.5f;
                        break;
                    }
                }
            }

            // Load the model (this is the blocking operation that was disturbing me)
            completed.model = std::make_shared<Model>(request.path);
            completed.success = true;
            completed.errorMessage = "";

            // std::cout << "[AssetLoader] Model loaded successfully: " << request.path << " model=" << (bool)completed.model << std::endl;
        }
        catch (const std::exception &e)
        {
            completed.success = false;
            completed.errorMessage = std::string("Failed to load model: ") + e.what();
            std::cerr << "[AssetLoader] " << completed.errorMessage << std::endl;
        }
        catch (...)
        {
            completed.success = false;
            completed.errorMessage = "Unknown error loading model";
            std::cerr << "[AssetLoader] " << completed.errorMessage << std::endl;
        }

        // Queue completion callback
        {
            std::lock_guard<std::mutex> lock(m_CompletedMutex);
            // std::cout << "[AssetLoader] Queuing completed load: success=" << completed.success
            //           << " hasCallback=" << (bool)completed.modelCallback << " hasModel=" << (bool)completed.model << std::endl;
            m_CompletedLoads.push(completed);
        }
    }

    void AssetLoader::ProcessSceneLoad(const LoadRequest &request)
    {
        CompletedLoad completed;
        completed.type = AssetType::Scene;
        completed.isModelLoad = false;
        completed.genericCallback = request.callback;
        completed.scene = static_cast<Scene *>(request.userData);
        completed.path = request.path;
        completed.success = false;

        try
        {
            if (!std::filesystem::exists(request.path))
            {
                completed.errorMessage = "Scene file not found";
            }
            else
            {
                nlohmann::json sceneJson;
                if (!SceneSerializer::ParseSceneFile(request.path, sceneJson))
                {
                    completed.errorMessage = "Failed to parse scene file";
                }
                else
                {
                    completed.sceneJson = std::move(sceneJson);

                    // Preload unique models in worker thread to avoid main-thread hitch.
                    std::unordered_set<std::string> uniqueModelPaths;
                    if (completed.sceneJson.contains("entities"))
                    {
                        for (const auto &entityJson : completed.sceneJson["entities"])
                        {
                            if (entityJson.contains("mesh") && entityJson["mesh"].contains("modelPath"))
                            {
                                std::string modelPath = entityJson["mesh"]["modelPath"].get<std::string>();
                                if (!modelPath.empty())
                                    uniqueModelPaths.insert(modelPath);
                            }
                        }
                    }

                    for (const auto &modelPath : uniqueModelPaths)
                    {
                        try
                        {
                            completed.preloadedModels[modelPath] = std::make_shared<Model>(modelPath);
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "[AssetLoader] Failed to preload model '" << modelPath << "': " << e.what() << std::endl;
                        }
                    }

                    completed.success = true;
                }
            }
        }
        catch (const std::exception &e)
        {
            completed.success = false;
            completed.errorMessage = std::string("Failed to process scene load: ") + e.what();
        }
        catch (...)
        {
            completed.success = false;
            completed.errorMessage = "Unknown error processing scene load";
        }

        // Update progress to indicate background phase completed.
        {
            std::lock_guard<std::mutex> lock(m_ProgressMutex);
            for (auto &progress : m_CurrentProgress)
            {
                if (progress.assetPath == request.path)
                {
                    progress.progress = 0.9f;
                    break;
                }
            }
        }

        // Queue completion callback
        {
            std::lock_guard<std::mutex> lock(m_CompletedMutex);
            m_CompletedLoads.push(std::move(completed));
        }
    }

    void AssetLoader::ProcessSkyboxLoad(const LoadRequest &request)
    {
        CompletedLoad completed;
        completed.type = AssetType::Skybox;
        completed.isModelLoad = false;
        completed.genericCallback = request.callback;
        completed.scene = static_cast<Scene *>(request.userData);
        completed.path = request.path;
        completed.success = std::filesystem::exists(request.path);
        if (!completed.success)
            completed.errorMessage = "Skybox file not found";

        // Update progress to indicate background IO phase completed.
        {
            std::lock_guard<std::mutex> lock(m_ProgressMutex);
            for (auto &progress : m_CurrentProgress)
            {
                if (progress.assetPath == request.path)
                {
                    progress.progress = 0.9f;
                    break;
                }
            }
        }

        // Queue completion callback
        {
            std::lock_guard<std::mutex> lock(m_CompletedMutex);
            m_CompletedLoads.push(completed);
        }
    }
}
