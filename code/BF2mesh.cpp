
#define _CRT_SECURE_NO_DEPRECATE
#include <assert.h>
#include <vector>
#include <set>
#include <assimp/DefaultLogger.hpp>
#include "SkeletonMeshBuilder.h"
#include "BaseImporter.h"
#include "BF2mesh.h"

namespace Assimp {

    namespace BF2File {


        // loads mesh from file
        int bf2mesh::Load(const char *filename, const char *ext)
        {
            assert(filename != NULL && ext != NULL);
            // decide type
            if (strcmp(ext, "bundledmesh") == 0) type = BF2MESHTYPE_BUNDLEDMESH;
            else if (strcmp(ext, "skinnedmesh") == 0) type = BF2MESHTYPE_SKINNEDMESH;
            else if (strcmp(ext, "staticmesh") == 0) type = BF2MESHTYPE_STATICMESH;
            else return 1;

            // open file
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                printf("File \"%s\" not found.\n", filename);
                return 1;
            }

            // header
            fread(&head, sizeof(bf2head), 1, fp);
            printf("head start at %i\n", ftell(fp));
            printf(" u1: %i\n", head.u1);
            printf(" version: %i\n", head.version);
            printf(" u3: %i\n", head.u5);
            printf(" u4: %i\n", head.u5);
            printf(" u5: %i\n", head.u5);
            printf("head end at %i\n", ftell(fp));
            printf("\n");

            // unknown (1 byte)
            // stupid little byte that misaligns the entire file!
            fread(&u1, 1, 1, fp);
            printf("u1: %i\n", u1);
            printf("\n");
            // for BFP4F, the value is "1", so perhaps this is a version number as well
            if (u1 == 1) {
                isBFP4F = true;
                if (type == BF2MESHTYPE_BUNDLEDMESH) type = BF2MESHTYPE_BUNDLEDMESH_BFP4F;
            }
            // --- geom table ---------------------------------------------------------------------------

            printf("geom table start at %i\n", ftell(fp));

            // geomnum (4 bytes)
            fread(&geomnum, 4, 1, fp);
            printf(" geomnum: %i\n", geomnum);

            // geom table (4 bytes * groupnum)
            geom = new bf2geom[geomnum];
            for (int i = 0; i < geomnum; i++)
            {
                geom[i].type = type;
                geom[i].Read(fp, head.version);
            }

            printf("geom table end at %i\n", ftell(fp));
            printf("\n");

            // --- vertex attribute table -------------------------------------------------------------------------------

            printf("attrib block at %i\n", ftell(fp));

            // vertattribnum (4 bytes)
            fread(&vertattribnum, sizeof(vertattribnum), 1, fp);
            printf(" vertattribnum: %i\n", vertattribnum);

            // vertex attributes
            vertattrib = new bf2attrib[vertattribnum];
            fread(vertattrib, sizeof(bf2attrib)*vertattribnum, 1, fp);
            for (int i = 0; i < vertattribnum; i++)
            {
                printf(" attrib[%i]: %i %i %i %i\n", i, vertattrib[i].flag,
                    vertattrib[i].offset,
                    vertattrib[i].vartype,
                    vertattrib[i].usage);
            }

            printf("attrib block end at %i\n", ftell(fp));
            printf("\n");

            // --- vertices -----------------------------------------------------------------------------

            printf("vertex block start at %i\n", ftell(fp));

            fread(&vertformat, 4, 1, fp);
            fread(&vertstride, 4, 1, fp);
            fread(&vertnum, 4, 1, fp);
            printf(" vertformat: %i\n", vertformat);
            printf(" vertstride: %i\n", vertstride);
            printf(" vertnum: %i\n", vertnum);

            assert(vertnum > 0 && vertformat == 4);

            //vert = new int[vertnum * (vertstride / vertformat)];
            //fread(vert, vertnum*vertstride, 1, fp);
            /*for (int i = 0; i < vertnum; i++)
            {
                printf("vertex %i's 27: %i\n", i, vert[i*(vertstride / vertformat) + (24 / vertformat)] >> 24);
                printf("vertex %i's 26: %i\n", i, (vert[i*(vertstride / vertformat) + (24 / vertformat)] & 0x00FF0000) >> 16);
                printf("vertex %i's 25: %i\n", i, (vert[i*(vertstride / vertformat) + (24 / vertformat)] & 0x0000FF00) >> 8);
                printf("vertex %i's 24: %i\n", i, vert[i*(vertstride / vertformat) + (24 / vertformat)] & 0x000000FF);
            }*/
            vert = new bf2vertices;
            if (!(vert->Read(fp, vertattrib, vertattribnum, vertnum))) return 1;

            printf("vertex block end at %i\n", ftell(fp));
            printf("\n");

            // --- indices ------------------------------------------------------------------------------

            printf("index block start at %i\n", ftell(fp));

            fread(&indexnum, 4, 1, fp);
            printf(" indexnum: %i\n", indexnum);
            index = new unsigned short[indexnum];
            fread(index, 2 * indexnum, 1, fp);

            printf("index block end at %i\n", ftell(fp));
            printf("\n");

            // --- rigs -------------------------------------------------------------------------------

            // unknown (4 bytes)
            if (type != BF2MESHTYPE_SKINNEDMESH) {
                fread(&u2, 4, 1, fp); // always 8?
                printf("u2: %i\n", u2);
                printf("\n");
            }

            // rigs/nodes
            printf("nodes chunk start at %i\n", ftell(fp));
            for (int i = 0; i < geomnum; i++)
            {
                if (i > 0) printf("\n");
                printf(" geom %i start\n", i);
                for (int j = 0; j < geom[i].lodnum; j++)
                {
                    printf("  lod %i start\n", j);
                    geom[i].lod[j].ReadNodeData(fp, head.version);
                    printf("  lod %i end\n", j);
                }
                printf(" geom %i end\n", i);
            }
            printf("nodes chunk end at %i\n", ftell(fp));
            printf("\n");

            // --- geoms ------------------------------------------------------------------------------

            for (int i = 0; i < geomnum; i++)
            {
                printf("geom %i start at %i\n", i, ftell(fp));
                //geom[i].ReadMatData( fp, head.version );

                for (int j = 0; j < geom[i].lodnum; j++)
                {
                    printf(" lod %i start\n", j);
                    geom[i].lod[j].ReadMatData(fp, head.version);
                    printf(" lod %i end\n", j);
                }

                printf("geom %i block end at %i\n", i, ftell(fp));
                printf("\n");
            }

            // --- end of file -------------------------------------------------------------------------

            printf("done reading %i\n", ftell(fp));
            fseek(fp, 0, SEEK_END);
            printf("file size is %i\n", ftell(fp));
            printf("\n");

            // close file
            fclose(fp);
            fp = NULL;

            // success!
            return 0;
        }

        // ---------------------------------------------------------------------------------------------------------
        // Create assimp scene
        // pScene: the assimp aiScene pointer
        // name: the scene root suffix
        // boneNodesArray: The constructed bone structure(e.x. in .con file or .ske file) in array(for bundledmeshes and skinnedmeshes).
        // If empty, the function will create a default one.
        // Don't forget to merge the boneNodes to the scene after it's created.
        int bf2mesh::createScene(aiScene* pScene, std::string name, std::vector<std::vector<aiNode*>> boneNodesArray)
        {
            if (pScene->mRootNode) return 1;//root node already applied, do nothing...

            //prepare aiMesh and aiMaterial cache
            std::vector<aiMesh*> meshArray;
            std::vector<aiMaterial*> matArray;

            // Create the root node of the scene
            pScene->mRootNode = new aiNode;
            // Set the name of the root node
            std::string typeName;
            switch (type) {
            case BF2MESHTYPE_BUNDLEDMESH: typeName = "bundledmesh_"; break;
            case BF2MESHTYPE_SKINNEDMESH: typeName = "skinnedmesh_"; break;
            case BF2MESHTYPE_STATICMESH: typeName = "staticmesh_"; break;
            }
            pScene->mRootNode->mName.Set("root_"+typeName+name);

            std::vector<aiNode*> boneNodes;
            //create bones for bundledmeshs
            if (type == BF2MESHTYPE_BUNDLEDMESH || type == BF2MESHTYPE_BUNDLEDMESH_BFP4F)
            {
                if (boneNodesArray.size() == 0) // create a debug bone structure
                {
                    // get max bone id
                    int maxBoneNum = 1;//at least one bone should be created
                    for (int i = 0; i < geomnum; i++)
                    {
                        for (int j = 0; j < geom[i].lodnum; j++)
                        {
                            if (geom[i].lod[j].nodenum > maxBoneNum) maxBoneNum = geom[i].lod[j].nodenum;
                        }
                    }
                    boneNodes = CreateDebugBoneStructure(pScene->mRootNode, maxBoneNum);
                }
                else if (boneNodesArray.size() == 1)
                    boneNodes = boneNodesArray[0];
                else
                {
                    DefaultLogger::get()->warn("bf2mesh::createScene : Input boneNodesArray size()>1. Using the first element.");
                    boneNodes = boneNodesArray[0];
                }
            }

            //create geom nodes and its subnodes(lodnodes)
            for (int i = 0; i < geomnum; i++)
            {
                aiNode* geomNode = new aiNode;
                geomNode->mName.Set("geom" + std::to_string(i));
                pScene->mRootNode->addChildren(1, &geomNode);

                //create bones for skinnedmeshs
                if (type == BF2MESHTYPE_SKINNEDMESH)
                {
                    if (boneNodesArray.size() == 0) // create a debug bone structure
                    {
                        // get max bone id
                        int maxBoneId = 0;//at least one bone should be created
                        for (int j = 0; j < geom[i].lodnum; j++)
                        {
                            bf2lod* tmpLod = (geom[i].lod) + j;
                            for (int k = 0; k < tmpLod->rignum; k++)
                            {
                                bf2rig* tmpRig = tmpLod->rig + k;
                                for (int m = 0; m < tmpRig->bonenum; m++)
                                {
                                    if (tmpRig->bone[m].id > maxBoneId) maxBoneId = tmpRig->bone[m].id;
                                }
                            }
                        }
                        boneNodes = CreateDebugBoneStructure(geomNode, maxBoneId + 1);
                    }
                    else if (boneNodesArray.size() <= i)
                        boneNodes = boneNodesArray[boneNodesArray.size()-1];//use the last set of input boneNodes
                    else boneNodes = boneNodesArray[i];                    
                }

                //create lod nodes
                for (int j = 0; j < geom[i].lodnum; j++)
                {
                    bf2lod* tmpLod = (geom[i].lod)+j;
                    aiNode* lodNode = new aiNode;
                    lodNode->mName.Set("lod" + std::to_string(j));
                    geomNode->addChildren(1, &lodNode);

                    //std::vector<aiNode*> meshNodeArray;

                    if (type == BF2MESHTYPE_SKINNEDMESH)//use rig info for skinnedmeshes
                    {
                        assert(tmpLod->rignum <= 4); //bf2 skinnedmeshes only supports no more than 4 rigs in a lod
                        assert(tmpLod->rignum == tmpLod->matnum); //bf2 skinnedmeshes only supports 1 material per rig
                        //add mesh nodes
                        for (int k = 0; k < tmpLod->rignum; k++)
                        {
                            aiNode* meshNode = new aiNode;
                            meshNode->mName.Set("mesh" + std::to_string(k));
                            meshNode->mNumMeshes = 1;
                            meshNode->mMeshes = new unsigned int[meshNode->mNumMeshes];

                            //adjust bone transforms
                            bf2rig* tmpRig = tmpLod->rig + k;
                            for (int bi = 0; bi < tmpRig->bonenum; bi++)
                            {
                                matrix4 tmpMatrix = tmpRig->bone[bi].transform;
                                aiMatrix4x4 tmpTransformation(
                                    tmpMatrix.m[0][0],
                                    tmpMatrix.m[1][0],
                                    tmpMatrix.m[2][0],
                                    tmpMatrix.m[3][0],
                                    tmpMatrix.m[0][1],
                                    tmpMatrix.m[1][1],
                                    tmpMatrix.m[2][1],
                                    tmpMatrix.m[3][1],
                                    tmpMatrix.m[0][2],
                                    tmpMatrix.m[1][2],
                                    tmpMatrix.m[2][2],
                                    tmpMatrix.m[3][2],
                                    tmpMatrix.m[0][3],
                                    tmpMatrix.m[1][3],
                                    tmpMatrix.m[2][3],
                                    tmpMatrix.m[3][3]
                                );

                                //TODO: update bone transformations from top to bottom

                            }
                            
                            lodNode->addChildren(1, &meshNode);
                            //meshNodeArray.push_back(meshNode);

                        }//end of rig loop

                        //lodNode->mChildren[0]->mNumMeshes = tmpLod->matnum;
                        //lodNode->mChildren[0]->mMeshes = new unsigned int[tmpLod->matnum];
                    }
                    else//use node info
                    {
                        if (type==BF2MESHTYPE_BUNDLEDMESH) assert(tmpLod->nodenum <= 26); //bf2 bundledmeshes only supports no more than 26 nodes in a lod
                        //add mesh nodes
                        for (int k = 0; k < tmpLod->nodenum; k++)
                        {
                            aiNode* meshNode = new aiNode;
                            meshNode->mName.Set("mesh" + std::to_string(k));
                            //meshNode->mNumMeshes = tmpLod->matnum;
                            //meshNode->mMeshes = new unsigned int[tmpLod->matnum];
                            //use transform
                            matrix4* tmpNode = tmpLod->node + k;
                            if (tmpLod->node != nullptr) meshNode->mTransformation = aiMatrix4x4(
                                tmpNode->m[0][0],
                                tmpNode->m[1][0],
                                tmpNode->m[2][0],
                                tmpNode->m[3][0],
                                tmpNode->m[0][1],
                                tmpNode->m[1][1],
                                tmpNode->m[2][1],
                                tmpNode->m[3][1],
                                tmpNode->m[0][2],
                                tmpNode->m[1][2],
                                tmpNode->m[2][2],
                                tmpNode->m[3][2],
                                tmpNode->m[0][3],
                                tmpNode->m[1][3],
                                tmpNode->m[2][3],
                                tmpNode->m[3][3]
                            );
                            lodNode->addChildren(1, &meshNode);
                            //meshNodeArray.push_back(meshNode);
                            
                        }//end of node loop

                        // TODO: Now we just add everything to the first mesh node and transform it with bone nodes.
                        // Find a better idea!
                        lodNode->mChildren[0]->mNumMeshes = tmpLod->matnum;
                        lodNode->mChildren[0]->mMeshes = new unsigned int[tmpLod->matnum];
                    }//end of node/rig selection

                    //create materials and meshes
                    for (int m = 0; m < tmpLod->matnum; m++)
                    {
                        bf2mat* tmpMat = tmpLod->mat+m;
                        aiNode* meshNode = lodNode->mChildren[0];
                        if (type == BF2MESHTYPE_SKINNEDMESH) meshNode = lodNode->mChildren[m]; // for skinnedmeshes, apply material to mesh directly

                        //process material info
                        aiMaterial* aiMat = new aiMaterial;
                        //use technique and alphamode as name
                        aiString matName;
                        matName.Set(tmpMat->technique);
                        switch (tmpMat->alphamode) {
                        case 0:
                            break;
                        case 1:
                            matName.Append(" || alphablend");
                            break;
                        case 2:
                            matName.Append(" || alphatest");
                            break;
                        }
                        aiMat->AddProperty(&matName, AI_MATKEY_NAME);
                        //add textures
                        //TODO: use better assimp texture representation
                        for (int t = 0; t < tmpMat->mapnum; t++)
                        {
                            aiString tex(tmpMat->map[t]);
                            //if (t == 0)
                            //    aiMat->AddProperty(&tex, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE,0));
                            //else
                                aiMat->AddProperty(&tex, AI_MATKEY_TEXTURE(aiTextureType_UNKNOWN, t));
                        }

                        //create mesh
                        aiMesh* mesh = new aiMesh;
                        mesh->mNumVertices = tmpMat->vnum;
                        if (mesh->mNumVertices == 0) {
                            throw DeadlyImportError("BF2: no vertices");
                        }
                        else if (mesh->mNumVertices > AI_MAX_ALLOC(aiVector3D)) {
                            throw DeadlyImportError("BF2: Too many vertices, would run out of memory");
                        }
                        //add all vertex info
                        if (vert->position)
                        {
                            mesh->mVertices = new aiVector3D[mesh->mNumVertices];
                            float3* pVal = vert->position + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mVertices[vi].Set(pVal->x, pVal->y, pVal->z);
                                pVal++;
                            }
                        }
                        if (vert->normal)
                        {
                            mesh->mNormals = new aiVector3D[mesh->mNumVertices];
                            float3* pVal = vert->normal + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mNormals[vi].Set(pVal->x, pVal->y, pVal->z);
                                pVal++;
                            }
                        }
                        /*if (vert->tangent)
                        {
                            mesh->mTangents = new aiVector3D[mesh->mNumVertices];
                            float3* pVal = vert->tangent + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTangents[vi].Set(pVal->x, pVal->y, pVal->z);
                                pVal++;
                            }
                        }*/ //TODO: maybe include tangent and calculating bitangent later is better?
                        if (vert->uv1)
                        {
                            mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
                            float2* pVal = vert->uv1 + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTextureCoords[0][vi].Set(pVal->x, pVal->y, 0.0);
                                pVal++;
                            }
                        }
                        if (vert->uv2)
                        {
                            mesh->mTextureCoords[1] = new aiVector3D[mesh->mNumVertices];
                            float2* pVal = vert->uv2 + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTextureCoords[1][vi].Set(pVal->x, pVal->y, 0.0);
                                pVal++;
                            }
                            //TODO: some bundledmeshes with animatedUV material may have this channel. Merge it into uv1.
                        }
                        if (vert->uv3)
                        {
                            mesh->mTextureCoords[2] = new aiVector3D[mesh->mNumVertices];
                            float2* pVal = vert->uv3 + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTextureCoords[2][vi].Set(pVal->x, pVal->y, 0.0);
                                pVal++;
                            }
                        }
                        if (vert->uv4)
                        {
                            mesh->mTextureCoords[3] = new aiVector3D[mesh->mNumVertices];
                            float2* pVal = vert->uv4 + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTextureCoords[3][vi].Set(pVal->x, pVal->y, 0.0);
                                pVal++;
                            }
                        }
                        if (vert->uv5)
                        {
                            mesh->mTextureCoords[4] = new aiVector3D[mesh->mNumVertices];
                            float2* pVal = vert->uv5 + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                mesh->mTextureCoords[4][vi].Set(pVal->x, pVal->y, 0.0);
                                pVal++;
                            }
                        }
                        //process blendWeight for skinnedmeshes
                        std::vector<float> vertexWeightArray(tmpMat->vnum);
                        if (type == BF2MESHTYPE_SKINNEDMESH && vert->blendWeight)
                        {
                            float *pVal = vert->blendWeight + tmpMat->vstart;
                            for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                            {
                                //do something
                                vertexWeightArray[vi] = *pVal;
                                pVal++;
                            }
                        }
                        //process blendIndices channel for bundledmeshes/skinnedmeshes
                        if (vert->blendIndices)
                        {
                            color4* pVal = vert->blendIndices + tmpMat->vstart;
                            // For bundledmeshes:
                            // blendIndices.r means the node index with which it transfroms
                            // blendIndices.a means animatedUV type:
                                /*
                                key: [] index value

                                Right Side
                                - treads .50 r [5]
                                - rotate .71 ( r and b ) [3]
                                - translate .60 [4]

                                Left Side
                                - treads .40 r [6]
                                - rotate .91  ( r and b ) [1]
                                - translate .80 [2]
                                */
                            if (type == BF2MESHTYPE_BUNDLEDMESH || type == BF2MESHTYPE_BUNDLEDMESH_BFP4F)
                            {
                                std::vector<std::vector<unsigned int>> nodeIdxArray(boneNodes.size());
                                std::set<unsigned int> usedNodeIdxs;
                                for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                                {
                                    if (pVal->r >= boneNodes.size()) return 1;//error
                                    nodeIdxArray[pVal->r].push_back(vi);
                                    usedNodeIdxs.insert(pVal->r);
                                    //TODO: process pVal->a for animatedUV material
                                    pVal++;
                                }

                                //check if usedNodeIdxs.size()==tmpLod->nodenum(may not be neccessary)
                                //if (usedNodeIdxs.size() != tmpLod->nodenum) return 1;//error

                                //create bones for this mesh
                                mesh->mNumBones = static_cast<unsigned int>(usedNodeIdxs.size());//(boneNodes.size()); //Could use tmpLod->nodenum here, but needs a bit more work
                                mesh->mBones = new aiBone*[mesh->mNumBones];
                                //for (unsigned int bi = 0; bi < mesh->mNumBones; bi++)
                                unsigned int bi = 0;
                                for (auto it = usedNodeIdxs.begin(); it != usedNodeIdxs.end(); it++)
                                {
                                    aiBone* nodeBone = new aiBone;
                                    unsigned int boneId = *it;
                                    if (boneId >= boneNodes.size())
                                    {
                                        DefaultLogger::get()->error("ERROR: boneId >= boneNodes.size() when importing BF2 Bundledmesh.");
                                        return 1;
                                    }
                                    nodeBone->mName = boneNodes[boneId]->mName;
                                    // calculate the bone offset matrix by concatenating the inverse transformations of all parents
                                    nodeBone->mOffsetMatrix = aiMatrix4x4(boneNodes[boneId]->mTransformation).Inverse();
                                    for (aiNode* parent = boneNodes[boneId]->mParent; parent != NULL; parent = parent->mParent)
                                        nodeBone->mOffsetMatrix = aiMatrix4x4(parent->mTransformation).Inverse() * nodeBone->mOffsetMatrix;

                                    //vertex weights
                                    nodeBone->mNumWeights = static_cast<unsigned int>(nodeIdxArray[boneId].size());
                                    if (nodeBone->mNumWeights > 0)
                                    {
                                        nodeBone->mWeights = new aiVertexWeight[nodeBone->mNumWeights];
                                        for (unsigned int wi = 0; wi < nodeBone->mNumWeights; wi++)
                                        {
                                            nodeBone->mWeights[wi].mVertexId = nodeIdxArray[boneId][wi];
                                            nodeBone->mWeights[wi].mWeight = 1.0;
                                        }
                                    }

                                    mesh->mBones[bi++] = nodeBone;
                                }
                            }
                            // For skinnedmeshes:
                            // blendIndices.r means the first bone index with weight $blendWeight for this vertex.
                            // blendIndices.g means the second bone index with weight (1-$blendWeight) for this vertex.
                            else if (type == BF2MESHTYPE_SKINNEDMESH)
                            {
                                bf2rig* tmpRig = tmpLod->rig + m;
                                std::vector<std::vector<unsigned int>> nodeIdxArray(tmpRig->bonenum);
                                std::vector<std::vector<unsigned int>> nodeIdxSecondArray(tmpRig->bonenum);
                                std::set<unsigned int> usedNodeIdxs;
                                for (unsigned int vi = 0; vi < tmpMat->vnum; vi++)
                                {
                                    nodeIdxArray[pVal->r].push_back(vi);
                                    if (pVal->r!= pVal->g) nodeIdxSecondArray[pVal->g].push_back(vi);
                                    usedNodeIdxs.insert(pVal->r);
                                    usedNodeIdxs.insert(pVal->g);
                                    pVal++;
                                }

                                //check if usedNodeIdxs.size()==tmpRig->bonenum(may not be neccessary)
                                //if (usedNodeIdxs.size() != tmpRig->bonenum) printf("usedNodeIdxs.size()!=tmpRig->bonenum");//return 1;//error

                                //create bones for this mesh
                                mesh->mNumBones = static_cast<unsigned int>(tmpRig->bonenum);//(usedNodeIdxs.size());
                                mesh->mBones = new aiBone*[mesh->mNumBones];
                                for (unsigned int bi = 0; bi < mesh->mNumBones; bi++)
                                //unsigned int bi = 0;
                                //for (auto it = usedNodeIdxs.begin(); it != usedNodeIdxs.end(); it++)
                                {
                                    aiBone* nodeBone = new aiBone;
                                    unsigned int boneId = tmpRig->bone[bi].id;//*it
                                    if (boneId >= boneNodes.size())
                                    {
                                        DefaultLogger::get()->warn("WARNING: boneId >= boneNodes.size() when importing BF2 Skinnedmesh. Using bone 0.");
                                        boneId = 0;
                                    }
                                    nodeBone->mName = boneNodes[boneId]->mName;
                                    // calculate the bone offset matrix by concatenating the inverse transformations of all parents
                                    nodeBone->mOffsetMatrix = aiMatrix4x4(boneNodes[boneId]->mTransformation).Inverse();
                                    for (aiNode* parent = boneNodes[boneId]->mParent; parent != NULL; parent = parent->mParent)
                                        nodeBone->mOffsetMatrix = aiMatrix4x4(parent->mTransformation).Inverse() * nodeBone->mOffsetMatrix;

                                    //vertex weights
                                    nodeBone->mNumWeights = static_cast<unsigned int>(nodeIdxArray[bi].size() + nodeIdxSecondArray[bi].size());
                                    if (nodeBone->mNumWeights > 0)
                                    {
                                        nodeBone->mWeights = new aiVertexWeight[nodeBone->mNumWeights];
                                        for (unsigned int wi = 0; wi < nodeIdxArray[bi].size(); wi++)
                                        {
                                            nodeBone->mWeights[wi].mVertexId = nodeIdxArray[bi][wi];
                                            nodeBone->mWeights[wi].mWeight = vertexWeightArray[nodeIdxArray[bi][wi]];
                                        }
                                        unsigned int wiOffset = static_cast<unsigned int>(nodeIdxArray[bi].size());
                                        for (unsigned int wi = 0; wi < nodeIdxSecondArray[bi].size(); wi++)
                                        {
                                            nodeBone->mWeights[wiOffset+wi].mVertexId = nodeIdxSecondArray[bi][wi];
                                            nodeBone->mWeights[wiOffset+wi].mWeight = 1.f - vertexWeightArray[nodeIdxSecondArray[bi][wi]];
                                        }
                                    }

                                    mesh->mBones[bi] = nodeBone;
                                    //bi++;
                                }
                            }
                        }
                        

                        //process faces
                        mesh->mNumFaces = tmpMat->inum / 3;
                        mesh->mFaces = new aiFace[mesh->mNumFaces];
                        unsigned short* pInd = index + tmpMat->istart;
                        for (unsigned int fi = 0; fi < mesh->mNumFaces; fi++)
                        {
                            mesh->mFaces[fi].mNumIndices = 3; // all triangles
                            mesh->mFaces[fi].mIndices = new unsigned int[3];
                            mesh->mFaces[fi].mIndices[0] = (unsigned int)pInd[0];
                            mesh->mFaces[fi].mIndices[1] = (unsigned int)pInd[1];
                            mesh->mFaces[fi].mIndices[2] = (unsigned int)pInd[2];
                            pInd += 3;
                        }

                        // Finally, add mesh and material to the global array
                        matArray.push_back(aiMat);
                        meshArray.push_back(mesh);
                        // And apply material to mesh && apply mesh to node
                        int matIndex = static_cast<unsigned int>(matArray.size() - 1);//also meshIndex
                        mesh->mMaterialIndex = matIndex;

                        if (type == BF2MESHTYPE_SKINNEDMESH) meshNode->mMeshes[0] = matIndex;
                        else meshNode->mMeshes[m] = matIndex;
                        //meshNode->mMeshes[m] = matIndex;
                    }//end of material loop

                }//end of lod loop
            }//end of geom loop

            // Finally, add/copy meshes and materials to pScene
            pScene->mNumMaterials = static_cast<unsigned int>(matArray.size());
            //if (pScene->mNumMaterials>0) pScene->mMaterials = &matArray[0];//matArray.data();
            if (pScene->mNumMaterials > 0)
            {
                pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials];
                for (unsigned int i = 0; i < pScene->mNumMaterials; i++)
                {
                    pScene->mMaterials[i] = matArray[i];
                }
            }
            pScene->mNumMeshes = static_cast<unsigned int>(meshArray.size());
            //if (pScene->mNumMeshes>0) pScene->mMeshes = &meshArray[0];//meshArray.data();
            if (pScene->mNumMeshes > 0)
            {
                pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
                for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
                {
                    pScene->mMeshes[i] = meshArray[i];
                }
            }
            
            return 0;
        }

        // reads rig
        bool bf2rig::Read(FILE *fp, int version)
        {

            // bonenum (4 bytes)
            fread(&bonenum, 4, 1, fp);
            printf("   bonenum: %i\n", bonenum);
            assert(bonenum >= 0);
            assert(bonenum < 99);

            // bones (68 bytes * bonenum)
            if (bonenum > 0) {
                bone = new bf2bone[bonenum];
                fread(bone, sizeof(bf2bone)*bonenum, 1, fp);

                for (int i = 0; i < bonenum; i++)
                {
                    printf("   boneid[%i]: %i\n", i, bone[i].id);
                }
            }

            // success
            return true;
        }


        // reads lod node table
        bool bf2lod::ReadNodeData(FILE *fp, int version)
        {

            // bounds (24 bytes)
            fread(&min, 12, 1, fp);
            fread(&max, 12, 1, fp);

            // unknown (12 bytes)
            if (version <= 6) { // version 4 and 6
                fread(&pivot, 12, 1, fp);
            }

            // skinnedmesh has different rigs
            if (type==BF2MESHTYPE_SKINNEDMESH) {

                // rignum (4 bytes)
                fread(&rignum, 4, 1, fp);
                printf("  rignum: %i\n", rignum);

                // read rigs
                if (rignum > 0) {
                    rig = new bf2rig[rignum];
                    for (int i = 0; i < rignum; i++)
                    {
                        printf("  rig block %i start at %i\n", i, ftell(fp));

                        rig[i].Read(fp, version);

                        printf("  rig block %i end at %i \n", i, ftell(fp));
                    }
                }

            }
            else {

                // nodenum (4 bytes)
                fread(&nodenum, 4, 1, fp);
                printf("   nodenum: %i\n", nodenum);

                // node matrices (64 bytes * nodenum) (BFP4F variant)
                if (type != BF2MESHTYPE_BUNDLEDMESH) {
                    printf("   node data\n");
                    if (nodenum > 0) {
                        node = new matrix4[nodenum];
                        //fread(node, sizeof(matrix4)*nodenum, 1, fp);
                        for (int i = 0; i < nodenum; i++)
                        {
                            fread(node+i, sizeof(matrix4), 1, fp);
                            if (type == BF2MESHTYPE_BUNDLEDMESH_BFP4F)
                            {
                                printf("   node string: %s\n", BF2ReadString(fp).c_str());
                            }
                        }      
                    }
                }

            }

            // success
            return true;
        }


        // reads lod material chunk
        bool bf2mat::Read(FILE *fp, int version)
        {

            // alpha flag (4 bytes)
            if (type != BF2MESHTYPE_SKINNEDMESH) {
                fread(&alphamode, 4, 1, fp);
                printf("   alphamode: %i\n", alphamode);
            }

            // fx filename
            fxfile = BF2ReadString(fp);
            printf("   fxfile: %s\n", fxfile.c_str());

            // material name
            technique = BF2ReadString(fp);
            printf("   matname: %s\n", technique.c_str());

            // mapnum (4 bytes)
            fread(&mapnum, 4, 1, fp);
            printf("   mapnum: %i\n", mapnum);
            assert(mapnum >= 0);
            assert(mapnum < 99);

            // mapnames
            if (mapnum > 0) {
                map = new std::string[mapnum];
                for (int i = 0; i < mapnum; i++)
                {
                    map[i] = BF2ReadString(fp);
                    printf("    map %i: %s\n", i, map[i].c_str());
                }
            }

            // geometry info
            fread(&vstart, 4, 1, fp);
            fread(&istart, 4, 1, fp);
            fread(&inum, 4, 1, fp);
            fread(&vnum, 4, 1, fp);
            printf("   vstart: %i\n", vstart);
            printf("   istart: %i\n", istart);
            printf("   inum: %i\n", inum);
            printf("   vnum: %i\n", vnum);

            // unknown
            fread(&u4, 4, 1, fp);
            fread(&u5, 2, 1, fp);
            fread(&u6, 2, 1, fp);

            // bounds
            if (type != BF2MESHTYPE_SKINNEDMESH) {
                if (version == 11) {
                    fread(&bounds, sizeof(aabb), 1, fp);
                }
            }

            // success
            return true;
        }


        // reads geom lod chunk
        bool bf2lod::ReadMatData(FILE *fp, int version)
        {
            // matnum (4 bytes)
            fread(&matnum, 4, 1, fp);
            printf("  matnum: %i\n", matnum);

            assert(matnum >= 0);
            assert(matnum < 99);

            // materials (? bytes)
            if (matnum > 0) {
                mat = new bf2mat[matnum];
                for (int i = 0; i < matnum; i++)
                {
                    mat[i].type = type;
                    printf("  mat %i start at %i\n", i, ftell(fp));
                    if (!mat[i].Read(fp, version)) return false;
                    printf("  mat %i end at %i\n", i, ftell(fp));
                }
            }

            // success
            return true;
        }


        // reads geom from file
        bool bf2geom::Read(FILE *fp, int version)
        {
            // lodnum (4 bytes)
            fread(&lodnum, 4, 1, fp);
            printf("  lodnum: %i\n", lodnum);

            assert(lodnum >= 0);
            assert(lodnum < 99);

            // allocate lods
            if (lodnum > 0) {
                lod = new bf2lod[lodnum];
                for (int i = 0; i < lodnum; i++)
                {
                    lod[i].type = type;
                }
            }

            // success
            return true;
        }

        bool bf2vertices::Read(FILE * fp, bf2attrib attribList[], int attribNum, int vertnum)
        {
            //read attribute list and initialize
            uintptr_t* offsetPtrs = new uintptr_t[attribNum-1];//pointer that used by fread to store data
            uintptr_t* increments = new uintptr_t[attribNum-1];//size of the pointer's data
            for (int i = 0; i < attribNum-1; i++) // the last attribute has non-zero flag
            {
                assert(attribList[i].flag==0); //check flag
                //TODO: check attribList[i].offset
                switch (attribList[i].usage)
                {
                case USAGE_POSITION:
                    assert(attribList[i].vartype == TYPE_FLOAT3);
                    position = new float3[vertnum];
                    offsetPtrs[i] = (uintptr_t)position;
                    increments[i] = sizeof(float3);
                    break;
                case USAGE_BLENDWEIGHT:
                    assert(attribList[i].vartype == TYPE_FLOAT1);
                    blendWeight = new float[vertnum];
                    offsetPtrs[i] = (uintptr_t)blendWeight;
                    increments[i] = sizeof(float);
                    break;
                case USAGE_BLENDINDICES:
                    assert(attribList[i].vartype == TYPE_D3DCOLOR);
                    blendIndices = new color4[vertnum];
                    offsetPtrs[i] = (uintptr_t)blendIndices;
                    increments[i] = sizeof(color4);
                    break;
                case USAGE_NORMAL:
                    assert(attribList[i].vartype == TYPE_FLOAT3);
                    normal = new float3[vertnum];
                    offsetPtrs[i] = (uintptr_t)normal;
                    increments[i] = sizeof(float3);
                    break;
                case USAGE_UV1:
                    assert(attribList[i].vartype == TYPE_FLOAT2);
                    uv1 = new float2[vertnum];
                    offsetPtrs[i] = (uintptr_t)uv1;
                    increments[i] = sizeof(float2);
                    break;
                case USAGE_TANGENT:
                    assert(attribList[i].vartype == TYPE_FLOAT3);
                    tangent = new float3[vertnum];
                    offsetPtrs[i] = (uintptr_t)tangent;
                    increments[i] = sizeof(float3);
                    break;
                case USAGE_UV2:
                    assert(attribList[i].vartype == TYPE_FLOAT2);
                    uv2 = new float2[vertnum]; 
                    offsetPtrs[i] = (uintptr_t)uv2;
                    increments[i] = sizeof(float2);
                    break;
                case USAGE_UV3:
                    assert(attribList[i].vartype == TYPE_FLOAT2);
                    uv3 = new float2[vertnum];
                    offsetPtrs[i] = (uintptr_t)uv3;
                    increments[i] = sizeof(float2);
                    break;
                case USAGE_UV4:
                    assert(attribList[i].vartype == TYPE_FLOAT2);
                    uv4 = new float2[vertnum];
                    offsetPtrs[i] = (uintptr_t)uv4;
                    increments[i] = sizeof(float2);
                    break;
                case USAGE_UV5:
                    assert(attribList[i].vartype == TYPE_FLOAT2);
                    uv5 = new float2[vertnum];
                    offsetPtrs[i] = (uintptr_t)uv5;
                    increments[i] = sizeof(float2);
                    break;
                default:
                    printf("Unknown vertex usage %d", attribList[i].usage);
                    return false;
                }
            }

            //read vertices
            for (int i = 0; i < vertnum; i++)
            {
                for (int j = 0; j < attribNum-1; j++)
                {
                    void* ptr = (void*)(offsetPtrs[j] + (increments[j]*i)); //the offset
                    fread(ptr, increments[j], 1, fp);
                }
            }

            return true;
        }

        int bf2col::Load(const char * filename, float impscale)
        {
            assert(filename != NULL);

            // open file
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                printf("File \"%s\" not found.\n", filename);
                return 1;
            }

            // --- head (8 bytes) ---------------------------------------------------------------------------
            printf("head start at %i\n", ftell(fp));
            fread(&head, 4, 1, fp);
            fread(&version, 4, 1, fp);
            printf(" head: %i\n", head);
            printf(" version: %i\n", version);
            printf("head end at %i\n", ftell(fp));

            // --- geoms ---------------------------------------------------------------------------

            printf("geom chunk start at %i\n", ftell(fp));

            // geomnum (4 bytes)
            fread(&geomNum, 4, 1, fp);
            printf(" geomNum: %i\n", geomNum);

            // geom chunk
            geoms = new bf2colGeom[geomNum];
            //find max material index
            for (int i = 0; i < geomNum; i++)
            {
                if(!geoms[i].Read(fp, version, impscale)) return 1;//may fail
                for (int j = 0; j < geoms[i].geomSubNum; j++)
                {
                    for (int k = 0; k < geoms[i].geomSubs[j].geomColNum; k++)
                    {
                        if (maxMatIdx < geoms[i].geomSubs[j].geomCols[k].maxMatIdx) maxMatIdx = geoms[i].geomSubs[j].geomCols[k].maxMatIdx;
                    }
                }
            }
            

            printf("geom chunk end at %i\n", ftell(fp));
            printf("\n");

            // --- end of file -------------------------------------------------------------------------

            printf("done reading %i\n", ftell(fp));
            fseek(fp, 0, SEEK_END);
            printf("file size is %i\n", ftell(fp));
            printf("\n");

            // close file
            fclose(fp);
            fp = NULL;

            // success!
            return 0;
        }

        static const aiColor3D colColorTable_static[10] = {
            aiColor3D(110.f/255,225.f/255,245.f/255),
            aiColor3D(245.f/255,110.f/255,225.f/255),
            aiColor3D(190.f/255,140.f/255,250.f/255),
            aiColor3D(170.f/255,250.f/255,110.f/255),
            aiColor3D(250.f/255,110.f/255,110.f/255),
            aiColor3D(250.f/255,170.f/255,110.f/255),
            aiColor3D(110.f/255,170.f/255,250.f/255),
            aiColor3D(250.f/255,220.f/255,110.f/255),
            aiColor3D(110.f/255,250.f/255,220.f/255),
            aiColor3D(250.f/255,110.f/255,140.f/255),
        };

        int bf2col::createScene(aiScene * pScene, std::string name)
        {
            if (pScene->mRootNode) return 1;//root node already applied, do nothing...

            //prepare aiMesh and aiMaterial cache
            std::vector<aiMesh*> meshArray;
            //std::vector<aiMaterial*> matArray;

            // Create the root node of the scene
            pScene->mRootNode = new aiNode;
            // Set the name of the root node
            pScene->mRootNode->mName.Set("root_collisionmesh_" + name);

            //int meshIndexOffset = 0; 
            //int maxMatIdx = 0;

            //create geom nodes and its subnodes(lodnodes)
            for (int i = 0; i < geomNum; i++)
            {
                aiNode* geomNode = new aiNode;
                geomNode->mName.Set("geom" + std::to_string(i));
                pScene->mRootNode->addChildren(1, &geomNode);

                //create geomsub nodes
                for (int j = 0; j < geoms[i].geomSubNum; j++)
                {
                    aiNode* geomSubNode = new aiNode;
                    geomSubNode->mName.Set("sub" + std::to_string(j));
                    geomNode->addChildren(1, &geomSubNode);
                    bf2GeomSub* tmpSub = geoms[i].geomSubs + j;

                    //int maxMatIdx = 0;
                    //get max material index
                    /*for (int k = 0; k < tmpSub->geomColNum; k++)
                    {
                        if (maxMatIdx < tmpSub->geomCols[k].maxMatIdx) maxMatIdx = tmpSub->geomCols[k].maxMatIdx;
                    }*/

                    //create col nodes
                    for (int k = 0; k < tmpSub->geomColNum; k++)
                    {
                        bf2GeomCol* tmpCol = tmpSub->geomCols+k;
                        if (tmpCol->vertNum == 0) continue;
                        aiNode* colNode = new aiNode;
                        colNode->mName.Set("col" + std::to_string(tmpCol->coltype));
                        geomSubNode->addChildren(1, &colNode);
                        
                        //create col materials and meshes
                        //create dummy vertList
                        aiVector3D* vertList;
                        //add vertices
                        if (tmpCol->vertNum == 0) {
                            throw DeadlyImportError("BF2: no vertices");
                        }
                        else if (tmpCol->vertNum > AI_MAX_ALLOC(aiVector3D)) {
                            throw DeadlyImportError("BF2: Too many vertices, would run out of memory");
                        }
                        else
                        {
                            vertList = new aiVector3D[tmpCol->vertNum];
                            float3* pVal = tmpCol->vertList;
                            for (int vi = 0; vi < tmpCol->vertNum; vi++)
                            {
                                vertList[vi].Set(pVal->x, pVal->y, pVal->z);
                                pVal++;
                            }
                        }

                        //count faces for all materials
                        int* faceCnt = new int[maxMatIdx + 1];
                        std::vector<unsigned int*>* faceListByMatIdx = new std::vector<unsigned int*> [maxMatIdx + 1];
                        //int validMatCnt = 0;
                        std::vector<unsigned short> validMatIds;
                        for (int m = 0; m < maxMatIdx + 1; m++)
                        {
                            faceCnt[m] = 0; // initialize
                        }
                        unsigned short* pInd = tmpCol->idxList;
                        for (int fi = 0; fi < tmpCol->faceNum; fi++)
                        {
                            unsigned short matIdx = pInd[3];
                            if (matIdx > maxMatIdx) return 1;// error condition!
                            unsigned int* tmpFaceIndices = new unsigned int[3];
                            tmpFaceIndices[0] = (unsigned int)pInd[0];
                            tmpFaceIndices[1] = (unsigned int)pInd[1];
                            tmpFaceIndices[2] = (unsigned int)pInd[2];
                            faceListByMatIdx[matIdx].push_back(tmpFaceIndices);
                            faceCnt[matIdx]++;
                            pInd += 4;
                        }
                        for (int m = 0; m < maxMatIdx + 1; m++)
                        {
                            if (faceCnt[m] > 0) validMatIds.push_back(m); // count valid materials for this col mesh
                        }

                        //process meshes
                        colNode->mNumMeshes = static_cast<unsigned int>(validMatIds.size());
                        colNode->mMeshes = new unsigned int[validMatIds.size()];
                        //aiMesh* tmpPMesh = pScene->mMeshes + meshIndexOffset;
                        for (int mId = 0; mId < validMatIds.size(); mId++)
                        {
                            aiMesh* mesh = new aiMesh();
                            //add vertList to meshes
                            mesh->mNumVertices = tmpCol->vertNum;
                            //mesh->mVertices = vertList; //sharing memory is not allowed?
                            //TODO: optimizes unused vertices(or will it be done in postprocess?)
                            mesh->mVertices = new aiVector3D[tmpCol->vertNum];
                            memcpy(mesh->mVertices, vertList, sizeof(aiVector3D)*mesh->mNumVertices);
                            mesh->mMaterialIndex = validMatIds[mId];
                            mesh->mNumFaces = faceCnt[mesh->mMaterialIndex];
                            if (mesh->mNumFaces>0) mesh->mFaces = new aiFace[mesh->mNumFaces];
                            std::vector<unsigned int*> faceList = faceListByMatIdx[validMatIds[mId]];
                            for (unsigned int fi = 0; fi < mesh->mNumFaces; fi++)
                            {
                                mesh->mFaces[fi].mNumIndices = 3;
                                mesh->mFaces[fi].mIndices = faceList[fi];
                            }

                            colNode->mMeshes[mId] = static_cast<unsigned int>(meshArray.size());
                            meshArray.push_back(mesh);
                        }

                        delete[] faceCnt;
                        delete[] faceListByMatIdx;
                    }//end of geomCol loop
                }//end of geomSub loop
            }//end of geom loop

            //finally create meshes and materials for pScene
            pScene->mNumMaterials = maxMatIdx + 1;
            pScene->mMaterials = new aiMaterial*[maxMatIdx + 1];
            //process materials
            for (int m = 0; m < maxMatIdx + 1; m++)
            {
                aiMaterial* aiMat = new aiMaterial;
                //use index as material name
                aiString matName;
                matName.Set(std::to_string(m));
                aiMat->AddProperty(&matName, AI_MATKEY_NAME);
                //add material color
                aiColor3D matColor;
                if (m < 10) matColor = colColorTable_static[m]; // use static color table
                else matColor = aiColor3D(1., 1., 1.);//TODO: use random color
                aiMat->AddProperty(&matColor, 1, AI_MATKEY_COLOR_AMBIENT);
                aiMat->AddProperty(&matColor, 1, AI_MATKEY_COLOR_DIFFUSE);
                aiMat->AddProperty(&matColor, 1, AI_MATKEY_COLOR_SPECULAR);
                pScene->mMaterials[m] = aiMat;
            }
            //pScene->mMeshes = new aiMesh*[maxMatIdx+1];

            pScene->mNumMeshes = static_cast<unsigned int>(meshArray.size());
            //if (pScene->mNumMeshes>0) pScene->mMeshes = &meshArray[0];//meshArray.data();
            if (pScene->mNumMeshes > 0)
            {
                pScene->mMeshes = new aiMesh*[pScene->mNumMeshes];
                for (unsigned int m = 0; m < pScene->mNumMeshes; m++)
                {
                    pScene->mMeshes[m] = meshArray[m];
                }
            }

            //success
            return 0;
        }

        bool bf2colGeom::Read(FILE * fp, int version, float impscale)
        {
            fread(&geomSubNum, 4, 1, fp);
            if (geomSubNum < 0) return false;
            printf(" geomSubNum: %i\n", geomSubNum);
            geomSubs = new bf2GeomSub[geomSubNum];
            for (int i = 0; i < geomSubNum; i++)
            {
                if (!geomSubs[i].Read(fp, version, impscale)) return false;
            }
            return true;
        }

        bool bf2GeomSub::Read(FILE * fp, int version, float impscale)
        {
            // --- cols ---------------------------------------------------------------------------

            printf("col chunk start at %i\n", ftell(fp));

            // geomColNum (4 bytes)
            fread(&geomColNum, 4, 1, fp);
            printf(" colNum: %i\n", geomColNum);

            if (geomColNum < 0) return false;

            // col chunk
            geomCols = new bf2GeomCol[geomColNum];
            for (int i = 0; i < geomColNum; i++)
            {
                if (!geomCols[i].Read(fp, version, impscale)) return false;
                //set coltype manually if version<9
                if (version < 9) geomCols->coltype = i;
            }

            printf("col chunk end at %i\n", ftell(fp));
            printf("\n");

            //success
            return true;
        }

        bool bf2GeomCol::Read(FILE * fp, int version, float impscale)
        {
            if (version >= 9)
            {
                fread(&coltype, 4, 1, fp);
            }
            // --- faces ------------------------------------------------------------------------------

            printf("face block start at %i\n", ftell(fp));

            fread(&faceNum, 4, 1, fp);
            printf(" faceNum: %i\n", faceNum);
            idxList = new unsigned short[4 * faceNum];
            fread(idxList, sizeof(unsigned short) * 4 * faceNum, 1, fp);
            for (int i = 3; i < 4 * faceNum; i += 4)
            {
                if (idxList[i] > maxMatIdx) maxMatIdx = idxList[i];
            }

            printf("face block end at %i\n", ftell(fp));
            printf("\n");

            // --- verts ------------------------------------------------------------------------------

            printf("vert block start at %i\n", ftell(fp));

            fread(&vertNum, 4, 1, fp);
            printf(" vertNum: %i\n", vertNum);
            vertList = new float3[vertNum];
            
            fread(vertList, sizeof(float3) * vertNum, 1, fp);
            for (int i = 0; i < vertNum; i++)
            {
                vertList[i].x *= impscale;
                vertList[i].y *= impscale;
                vertList[i].z *= impscale;
            }

            printf("vert block end at %i\n", ftell(fp));
            printf("\n");

            // --- unknowns ------------------------------------------------------------------------------
            uList = new unsigned short[vertNum];
            fread(uList, sizeof(unsigned short) * vertNum, 1, fp);
            fread(&uBB1, sizeof(aabb), 1, fp);
            fread(&ub, 1, 1, fp);
            fread(&uBB2, sizeof(aabb), 1, fp);
            fread(&ynum, 4, 1, fp);
            printf(" ynum: %i\n", ynum);
            yList = new unsigned int[4*ynum];
            fread(yList, sizeof(unsigned int) * 4 * ynum, 1, fp);
            fread(&znum, 4, 1, fp);
            printf(" znum: %i\n", znum);
            zList = new unsigned short[znum];
            fread(zList, sizeof(unsigned short) * znum, 1, fp);
            
            if (version >= 10)
            {
                fread(&anum, 4, 1, fp);
                printf(" anum: %i\n", anum);
                aList = new unsigned int[anum];
                fread(aList, sizeof(unsigned int) * anum, 1, fp);
            }

            return true;
        }

        bool bf2skeBone::Read(FILE * fp, int version, float scale)
        {
            name = readName(fp);
            fread(&motherIdx, 2, 1, fp);
            printf("bone: %s, motherIdx=%d\n", name.c_str(), motherIdx);
            fread(&rotation, sizeof(float4), 1, fp);
            fread(&position, sizeof(float3), 1, fp);
            if (name.substr(0, 4) == "mesh") //kit nodes
            {
                rotation = float4{ 0,0,0,1 };
                position = float3{ 0,0,0 };
            }
            printf("pos: %f,%f,%f\n", position.x, position.y, position.z);
            printf("rot: %f,%f,%f,%f\n", rotation.x, rotation.y, rotation.z, rotation.w);
            //success
            return true;
        }

        std::string bf2skeBone::readName(FILE * fp)
        {
            unsigned short num;
            fread(&num, 2, 1, fp);

            assert(num >= 0);
            assert(num < 99);

            if (num == 0) return "";

            char *str = new char[num];
            fread(str, num, 1, fp);

            std::string tmp(str, num);

            delete[] str;

            return tmp;
        }

        int bf2ske::Load(const char * filename, float scale)
        {
            assert(filename != NULL);

            // open file
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                printf("File \"%s\" not found.\n", filename);
                return 1;
            }

            // --- head (4 bytes) ---------------------------------------------------------------------------
            printf("head start at %i\n", ftell(fp));
            fread(&version, 4, 1, fp);
            printf(" version: %i\n", version);
            printf("head end at %i\n", ftell(fp));

            // --- bones ---------------------------------------------------------------------------
            printf("bones chunk start at %i\n", ftell(fp));

            // geomnum (4 bytes)
            fread(&boneNum, 4, 1, fp);
            //assert(boneNum <= 80);//in bf2, bone number cannot exceed 80(may exceed 80 in later games)
            printf(" boneNum: %i\n", boneNum);

            // geom chunk
            bones = new bf2skeBone[boneNum];
            //find max material index
            for (int i = 0; i < boneNum; i++)
            {
                if (!bones[i].Read(fp, version, scale)) return 1;//may fail
            }

            printf("bones chunk end at %i\n", ftell(fp));
            printf("\n");

            // --- end of file -------------------------------------------------------------------------

            printf("done reading %i\n", ftell(fp));
            fseek(fp, 0, SEEK_END);
            printf("file size is %i\n", ftell(fp));
            printf("\n");

            // close file
            fclose(fp);
            fp = NULL;

            // success!
            return 0;
        }

        int bf2ske::createScene(aiScene * pScene, std::string name)
        {
            if (pScene->mRootNode) return 1;//root node already applied, do nothing...

            // Create the root node of the scene
            pScene->mRootNode = new aiNode;
            // Set the name of the root node
            pScene->mRootNode->mName.Set("root_skeleton_" + name);

            //int meshIndexOffset = 0; 
            //int maxMatIdx = 0;

            //bone node array
            std::vector<aiNode*> boneNodeArray;

            //create bone nodes
            for (int i = 0; i < boneNum; i++)
            {
                aiNode* pBoneNode = new aiNode;
                pBoneNode->mName.Set(bones[i].name.c_str());//(std::to_string(i));
                pBoneNode->mTransformation = aiMatrix4x4(
                    aiVector3D(1.0, 1.0, 1.0), //scaling
                    aiQuaternion(bones[i].rotation.w, -bones[i].rotation.x, -bones[i].rotation.y, -bones[i].rotation.z), //rotation
                    //aiQuaternion(aiVector3D(bones[i].rotation.x, bones[i].rotation.y, bones[i].rotation.z), bones[i].rotation.w),
                    aiVector3D(bones[i].position.x, bones[i].position.y, bones[i].position.z) //position
                );
                boneNodeArray.push_back(pBoneNode);
            }

            // create topology
            for (int i = 0; i < boneNum; i++)
            {
                if (bones[i].motherIdx < 0) pScene->mRootNode->addChildren(1, &boneNodeArray[i]);
                else if (bones[i].motherIdx >= boneNodeArray.size()) return 1;//error condition
                else boneNodeArray[bones[i].motherIdx]->addChildren(1, &boneNodeArray[i]);
            }

            // create dummy mesh
            for (unsigned int i = 0; i < pScene->mRootNode->mNumChildren; i++)
            {
                //skip camera bone
                if (pScene->mRootNode->mChildren[i]->mName == aiString("Camerabone")) continue;

                //build skeleton
                SkeletonMeshBuilder skeleton(pScene, pScene->mRootNode->mChildren[i]);
                break; // can only build once
            }
            

            //success!
            return 0;
        }

        int bf2baf::Load(const char * filename, float scale)
        {
            assert(filename != NULL);

            // open file
            FILE *fp = fopen(filename, "rb");
            if (!fp) {
                printf("File \"%s\" not found.\n", filename);
                return 1;
            }

            // --- head (4 bytes) ---------------------------------------------------------------------------
            printf("head start at %i\n", ftell(fp));
            fread(&version, 4, 1, fp);
            printf(" version: %i\n", version);
            printf("head end at %i\n", ftell(fp));

            // --- bones ---------------------------------------------------------------------------
            // boneNum (2 bytes)
            fread(&boneNum, 2, 1, fp);
            //assert(boneNum <= 80);//in bf2, bone number cannot exceed 80(may exceed 80 in later games)
            printf(" boneNum: %i\n", boneNum);

            boneIds = new short[boneNum];
            fread(boneIds, 2*boneNum, 1, fp);

            // frameNum (4 bytes)
            fread(&frameNum, 4, 1, fp);
            printf(" frameNum: %i\n", frameNum);

            // precision (1 byte)
            fread(&filePrecision, 1, 1, fp);
            printf(" filePrecision: %i\n", filePrecision);

            bones = new bf2animbone[boneNum];
            for (int i = 0; i < boneNum; i++)
            {
                printf("bone %i start at %i\n",i ,ftell(fp));
                bones[i].frameNum = frameNum;
                bones[i].filePrecision = filePrecision;
                printf("bones %i id:%i \n", i, boneIds[i]);
                if (!bones[i].Read(fp, version, scale)) return 1;//may fail
                printf("bone %i end at %i\n", i, ftell(fp));
            }

            // --- end of file -------------------------------------------------------------------------

            printf("done reading %i\n", ftell(fp));
            fseek(fp, 0, SEEK_END);
            printf("file size is %i\n", ftell(fp));
            printf("\n");

            // close file
            fclose(fp);
            fp = NULL;

            // success!
            return 0;
        }

        aiAnimation* bf2baf::applyToNodes(std::string animName, std::vector<aiNode*> boneNodes)
        {
            //initialize animation
            aiAnimation* aiAnim = new aiAnimation;
            aiAnim->mName.Set(animName);
            aiAnim->mDuration = frameNum;
            aiAnim->mNumChannels = boneNum;
            aiAnim->mChannels = new aiNodeAnim*[aiAnim->mNumChannels];

            for (int i = 0; i < boneNum; i++)
            {
                //initialize node animation
                aiNodeAnim* nodeAnim = new aiNodeAnim;
                if (boneIds[i] >= boneNodes.size())
                {
                    DefaultLogger::get()->error("bf2baf::applyToNodes : boneIds[i]>=boneNodes.size()");
                    return NULL;
                }
                nodeAnim->mNodeName = boneNodes[boneIds[i]]->mName;
                nodeAnim->mNumPositionKeys = bones[i].frameNum;
                nodeAnim->mNumRotationKeys = bones[i].frameNum;
                nodeAnim->mPositionKeys = new aiVectorKey[nodeAnim->mNumPositionKeys];
                nodeAnim->mRotationKeys = new aiQuatKey[nodeAnim->mNumRotationKeys];

                for (int j = 0; j < bones[i].frameNum; j++)
                {
                    nodeAnim->mPositionKeys[j].mTime = j;
                    nodeAnim->mPositionKeys[j].mValue = aiVector3D(
                        bones[i].frames[j].pos.x,
                        bones[i].frames[j].pos.y,
                        bones[i].frames[j].pos.z
                    );
                    nodeAnim->mRotationKeys[j].mTime = j;
                    nodeAnim->mRotationKeys[j].mValue = aiQuaternion(
                        bones[i].frames[j].rot.w,
                        -bones[i].frames[j].rot.x,
                        -bones[i].frames[j].rot.y,
                        -bones[i].frames[j].rot.z
                    );
                }

                aiAnim->mChannels[i] = nodeAnim;
            }

            return aiAnim;
        }

        int bf2baf::createScene(aiScene * pScene, std::string name, std::vector<aiNode*> boneNodes)
        {
            if (pScene->mRootNode) return 1;//root node already applied, do nothing...
            // Create the root node of the scene
            pScene->mRootNode = new aiNode;
            // Set the name of the root node
            pScene->mRootNode->mName.Set("root_animation_" + name);

            
            int maxBoneId = static_cast<int>(boneNodes.size())-1;
            //create debug bone structure if no skeleton file is imported
            if (maxBoneId < 0)
            {
                for (int i = 0; i < boneNum; i++)
                {
                    if (maxBoneId < boneIds[i]) maxBoneId = boneIds[i];
                }
                boneNodes = CreateDebugBoneStructure(pScene->mRootNode, maxBoneId + 1);
            }

            aiAnimation* tmpAnim = applyToNodes(name, boneNodes);
            if (!tmpAnim) return 1;//error

            // finally, apply animation to the scene
            pScene->mNumAnimations = 1;
            pScene->mAnimations = new aiAnimation*[pScene->mNumAnimations];
            pScene->mAnimations[0] = tmpAnim;

            // As we have no meshes here, it's neccessary to create mesh using SkeletonMeshBuilder
            SkeletonMeshBuilder skeleton(pScene, pScene->mRootNode->mChildren[0]);

            // success!
            return 0;
        }

        bool bf2animbone::Read(FILE * fp, int version, float scale)
        {
            fread(&dataLenTotal, 2, 1, fp);
            printf(" dataLenTotal: %i\n", dataLenTotal);

            if (frameNum <= 0) {
                printf(" invalid frameNum %i\n", frameNum);
                return false;
            }

            frames = new bf2frame[frameNum];

            for (int i = 0; i < 7; i++)
            {
                short dataLeft;
                fread(&dataLeft, 2, 1, fp);
                int curFrame = 0;
                while (dataLeft > 0)
                {
                    int8_t tmpByte;
                    fread(&tmpByte, 1, 1, fp);
                    bool rleCompression = (tmpByte & 0b10000000) >> 7;
                    int8_t numFrames = (tmpByte & 0b01111111);
                    printf(" numFrames: %i\n", numFrames);
                    int8_t nextHeader;
                    fread(&nextHeader, 1, 1, fp);
                    short tmpVal;
                    if (rleCompression) fread(&tmpVal, 2, 1, fp);

                    //read numFrames times
                    for (int j = 0; j < numFrames; j++)
                    {
                        if (!rleCompression) fread(&tmpVal, 2, 1, fp);
                        switch (i)
                        {
                        //0-3 is rotation part
                        case 0: frames[curFrame].rot.x = Convert16bitToFloat(tmpVal, 15); break;
                        case 1: frames[curFrame].rot.y = Convert16bitToFloat(tmpVal, 15); break;
                        case 2: frames[curFrame].rot.z = Convert16bitToFloat(tmpVal, 15); break;
                        case 3: frames[curFrame].rot.w = Convert16bitToFloat(tmpVal, 15); break;
                        //4-6 is position part
                        case 4: frames[curFrame].pos.x = Convert16bitToFloat(tmpVal, filePrecision); break;
                        case 5: frames[curFrame].pos.y = Convert16bitToFloat(tmpVal, filePrecision); break;
                        case 6: frames[curFrame].pos.z = Convert16bitToFloat(tmpVal, filePrecision); break;
                            
                        }
                        curFrame++;
                    }

                    //decrement
                    dataLeft -= nextHeader;
                }
            }

            return true;
        }

        // reads string from file
        std::string BF2ReadString(FILE *fp)
        {
            unsigned int num;
            fread(&num, 4, 1, fp);

            assert(num >= 0);
            assert(num < 999);

            if (num == 0) return "";

            char *str = new char[num];
            fread(str, num, 1, fp);

            std::string tmp(str, num);

            delete[] str;

            return tmp;
        }

        float Convert16bitToFloat(int16_t tmpInt16, int8_t precision)
        {
            float flt16_mult = 32767.f / (2 << (15 - precision));
            float tmpVal = (float)tmpInt16;
            if (tmpInt16 > 32767) tmpVal -= 65535;
            return tmpVal / flt16_mult;
        }

        float Read16bitFloat(FILE * fp, int8_t precision)
        {
            int16_t tmpInt;
            fread(&tmpInt, 2, 1, fp);
            return Convert16bitToFloat(tmpInt, precision);
        }

        float3 Read16bitFloat3(FILE * fp, int8_t precision)
        {
            float3 tmpVal;
            tmpVal.x = Read16bitFloat(fp, precision);
            tmpVal.y = Read16bitFloat(fp, precision);
            tmpVal.z = Read16bitFloat(fp, precision);
            return tmpVal;
        }

        float4 Read16bitFloat4(FILE * fp, int8_t precision)
        {
            float4 tmpVal;
            tmpVal.x = Read16bitFloat(fp, precision);
            tmpVal.y = Read16bitFloat(fp, precision);
            tmpVal.z = Read16bitFloat(fp, precision);
            tmpVal.w = Read16bitFloat(fp, precision);
            return tmpVal;
        }

        // create a temporary bone structure for rig/animation and return the array of the bone nodes
        std::vector<aiNode*> CreateDebugBoneStructure(aiNode * root, int boneNum)
        {
            std::vector<aiNode*> boneArray;

            if (!root) return boneArray; // check root

            // a root for the bones
            aiNode* boneRootNode = new aiNode; 
            // Set the name of the root node
            boneRootNode->mName.Set("root_skeleton_debug");
            root->addChildren(1, &boneRootNode);

            // bone nodes
            for (int i = 0; i < boneNum; i++)
            {
                aiNode* boneNode = new aiNode;
                boneNode->mName.Set("bone" + std::to_string(i));
                //give the bones different transforms
                boneNode->mTransformation = aiMatrix4x4(
                    aiVector3D(1.0, 1.0, 1.0),
                    aiQuaternion(1.0, 0.0, 0.0, 0.0),
                    aiVector3D(0.0, 0.0, (ai_real)(i+1))
                );
                boneRootNode->addChildren(1, &boneNode);
                boneArray.push_back(boneNode);
            }

            return boneArray;
        }

}//namespace BF2File
}//namespace Assimp

