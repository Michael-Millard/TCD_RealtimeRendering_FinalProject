#ifndef MY_MODEL_H
#define MY_MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <my_mesh.h>
#include <my_shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

class Model
{
public:
    // Public for wall constraints
    std::vector<Mesh> meshes;

    // Constructor (expects a filepath to a 3D model)
    Model(std::string const& objPath, const std::string& modelName)
    {
        this->modelName = modelName;
        loadModel(objPath);
        printModelDetails();
    }

    // Draw the model (all its meshes)
    void draw(Shader& shader)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(meshes.size()); i++)
            meshes[i].draw(shader);
    }

private:
    std::string modelName;

    // Load a 3D model specified by path
    void loadModel(std::string const& path)
    {
        // Read file
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // Processes a node recursively
    void processNode(aiNode* node, const aiScene* scene)
    {
        // Process each mesh located at current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // Recursively process children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // Data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Loop through mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Normals (if it has)
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // Precomputed d_N vertex color attribute from Blender
            if (mesh->HasVertexColors(0))
                vertex.d_N = mesh->mColors[0][i].r; // Assuming r = g = b = d_N
            else 
                vertex.d_N = 0.0f; // Fallback value        

            vertices.push_back(vertex);
        }

        // Loop through mesh's faces and retrieve the corresponding vertex indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            // Retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Return a mesh object created from the extracted mesh data
        Mesh tempMesh(vertices, indices);

        // Set name if present
        std::string meshName = std::string(mesh->mName.C_Str());
        if (!meshName.empty())
            tempMesh.meshName = meshName;

        return tempMesh;
    }

    void printModelDetails()
    {
        unsigned int totalVertices = 0;
        unsigned int totalTriangles = 0;

        for (const auto& mesh : meshes)
        {
            totalVertices += static_cast<unsigned int>(mesh.vertices.size());
            totalTriangles += static_cast<unsigned int>(mesh.indices.size()) / 3;
        }

        std::cout << "****************************\n";
        std::cout << "Successfully Loaded Model: " << modelName << "\n";
        std::cout << "Model contains " << meshes.size() << " mesh(es).\n";
        std::cout << "Total vertices: " << totalVertices << "\n";
        std::cout << "Total triangles: " << totalTriangles << "\n";
        std::cout << "****************************\n\n";
    }
};

#endif // MY_MODEL_H
