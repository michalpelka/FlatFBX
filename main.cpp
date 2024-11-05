#include <fbxsdk.h>
#include <functional>
#include <iostream>
#include <vector>
#include <cassert>
#include <string_view>
#include <filesystem>
#include <set>
#include <fstream>
void Loop(FbxNode *node, std::function<void(const FbxNode *, int)> fun,
          int level = 0) {
    if (!node)
        return;
    fun(node, level);
    level++;
    for (int i = 0; i < node->GetChildCount(); ++i) {
        FbxNode *childNode = node->GetChild(i);
        if (!childNode)
            continue;

        Loop(childNode, fun, level);
    }
}

// Clone and add a node to the target scene
FbxNode *CloneNodeToScene(FbxNode *sourceNode, FbxScene *targetScene) {
    if (!sourceNode || !targetScene) return nullptr;

    auto translation = sourceNode->GetGeometricTranslation(fbxsdk::FbxNode::eSourcePivot);
    auto rot = sourceNode->GetGeometricRotation(fbxsdk::FbxNode::eSourcePivot);
    auto scaling = sourceNode->GetGeometricScaling(fbxsdk::FbxNode::eSourcePivot);
    // Clone the node
    FbxNode *clonedNode = FbxNode::Create(targetScene, sourceNode->GetName());

    clonedNode->LclTranslation.Set(translation);
    clonedNode->LclRotation.Set(rot);
    clonedNode->LclScaling.Set(scaling);

    // Copy node attributes if they exist (e.g., mesh, light)
    FbxNodeAttribute *attr = sourceNode->GetNodeAttribute();
    if (attr) {
        FbxObject *clonedAttr = attr->Clone(FbxObject::eDeepClone, targetScene);
        FbxNodeAttribute *FBclonedAttr = FbxCast<FbxNodeAttribute>(clonedAttr);
        assert(FBclonedAttr);
        clonedNode->SetNodeAttribute(FBclonedAttr);
    }


    // Copy materials
    for (int i = 0; i < sourceNode->GetMaterialCount(); ++i) {
        FbxSurfaceMaterial *material = sourceNode->GetMaterial(i);
        if (material) {
            FbxSurfaceMaterial *clonedMaterial = FbxCast<FbxSurfaceMaterial>(
                    material->Clone(FbxObject::eDeepClone, targetScene));
            clonedNode->AddMaterial(clonedMaterial);
        }
    }

    return clonedNode;
}


void CreateFBXScene(FbxManager *sdkManager, FbxScene* orginalScene, const std::string& outFilename, std::vector<FbxNode *>::iterator start, std::vector<FbxNode *>::iterator end)
{
    std::cout << "Creating FBX scene " << outFilename << "number of nodes " << std::distance(start,end) <<"\n";
    const std::filesystem::path p(outFilename);
    const std::string basename = p.stem().string();

    FbxScene *exportScene = FbxScene::Create(sdkManager, "");
    auto axisSystem = orginalScene->GetGlobalSettings().GetAxisSystem();
    exportScene->GetGlobalSettings().SetAxisSystem(axisSystem);
    FbxExporter *exporter = FbxExporter::Create(sdkManager, "");
    auto rootNode = exportScene->GetRootNode();
    auto baseNode = rootNode->Create(exportScene->GetRootNode(), basename.c_str());

    if (!exporter->Initialize(outFilename.c_str(), -1, sdkManager->GetIOSettings())) {
        std::cerr << "Error: Unable to save the modified FBX to " << outFilename << "\n";
        std::abort();
    }

    for (auto nodeIt = start; nodeIt != end; ++nodeIt) {
        FbxNode *clonedNode = CloneNodeToScene(*nodeIt, exportScene);
        baseNode->AddChild(clonedNode);
    }
    exporter->Export(exportScene);
    exporter->Destroy();

}

// Main function
int main(int argc, char **argv) {

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <FBX file in> <FBX file out>\n";
        return -1;
    }

    std::set<std::string> flags;
    for (int i = 0; i < argc; ++i) {
        if (std::string_view(argv[i]).front() == '-') {
            flags.insert(argv[i]);
        }
    }

    bool force = false;
    bool dryRun = false;
    bool split = false;
    if (flags.find("-f") != flags.end()) {
        force = true;
    }
    if (flags.find("-d") != flags.end()) {
        dryRun = true;
    }
    if (flags.find("-s") != flags.end()) {
        split = true;
    }



    const std::string inFilename{argv[1]};
    const std::string outFilename{argv[2]};

    if (inFilename == outFilename) {
        std::cerr << "Error: Input and output filenames are the same.\n";
        return -1;
    }

    if (inFilename.find(".fbx") == std::string::npos || outFilename.find(".fbx") == std::string::npos) {
        std::cerr << "Error: Input and output filenames must have the .fbx extension.\n";
        return -1;
    }

    if (!force && std::filesystem::exists(outFilename)) {
        std::cerr << "Error: Output file already exists, skipping.\n";
        return 0;
    }

    FbxManager *sdkManager = FbxManager::Create();
    if (!sdkManager) {
        std::cerr << "Error: Unable to create FBX Manager.\n";
        return -1;
    }

    FbxIOSettings *ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
    sdkManager->SetIOSettings(ioSettings);

    FbxImporter *importer = FbxImporter::Create(sdkManager, "");

    if (!importer->Initialize(inFilename.c_str(), -1, sdkManager->GetIOSettings())) {
        std::cerr << "Error: Unable to open file " << inFilename << "\n";
        return -1;
    }

    FbxScene *scene = FbxScene::Create(sdkManager, "myScene");

    importer->Import(scene);
    importer->Destroy();

    FbxNode *rootNode = scene->GetRootNode();
    if (rootNode) {
        std::cout << "Root node name: " << rootNode->GetName() << "\n";
    }

    std::vector<FbxNode *> usableNodes;
    int allNodesCount = 0;
    int maxLevel = 0;

    Loop(rootNode, [&](const FbxNode *node, int level) {
        allNodesCount++;
        if (strncmp(node->GetTypeName(), "Mesh", 4) == 0) {
            maxLevel = std::max(maxLevel, level);
            usableNodes.push_back(const_cast<FbxNode *>(node));
        }
    });
    std::cout << "Number of usable nodes : " << usableNodes.size() << "\n";
    std::cout << "Number of all nodes : " << allNodesCount << "\n";
    std::cout << "Max FBX depth : " << maxLevel << "\n";

    if (dryRun) {
        std::cout << "Dry run, exiting.\n";
        return 0;
    }

    constexpr size_t maxEntities = 5000;
    if (!split) {
        CreateFBXScene(sdkManager, scene, outFilename, usableNodes.begin(), usableNodes.end());
    }
    else{
        size_t currentNode =0;
        for(;;)
        {
            if(currentNode >= usableNodes.size())
            {
                break;
            }
            const size_t endElementIndex = std::min(currentNode+maxEntities, usableNodes.size());

            char fName[256];
            snprintf(fName, 256, "_%07zu.fbx", currentNode);
            std::string newFileName = outFilename;
            std::filesystem::path path(outFilename);
            std::filesystem::path newPath  = path.parent_path()/(path.stem().string()+std::string(fName));

            CreateFBXScene(sdkManager, scene, newPath.string(), usableNodes.begin() + currentNode, usableNodes.begin() + endElementIndex);
            currentNode += maxEntities;

        }

    }

    sdkManager->Destroy();
    std::cout << "FBX file processed and saved successfully.\n";

    return 0;
}