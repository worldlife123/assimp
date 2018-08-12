#ifndef BF2_FILE_IMPORTER_H_INC
#define BF2_FILE_IMPORTER_H_INC

#include "BaseImporter.h"
#include "Importer.h"
#include <vector>

#include "BF2mesh.h"

struct aiMesh;
struct aiNode;

namespace Assimp {

namespace BF2File {

    // ------------------------------------------------------------------------------------------------
    //! \struct Object
    //! \brief  Stores all bf2 objects of an .con file object definition
    // ------------------------------------------------------------------------------------------------
    struct Object {
        /*enum ObjectType {
            ObjType,
            GroupType
        };*/

        //! Object name
        std::string m_strObjName;
        //! Transformation matrix, stored in OpenGL format
        aiMatrix4x4 m_Transformation;
        //! All sub-objects referenced by this object
        std::vector<Object*> m_SubObjects;
        /// Assigned geometry(mesh)
        std::vector<unsigned int> m_Geometry;

        //! \brief  Default constructor
        Object()
            : m_strObjName("") {
            // empty
        }

        //! \brief  Destructor
        ~Object() {
            for (std::vector<Object*>::iterator it = m_SubObjects.begin(); it != m_SubObjects.end(); ++it) {
                delete *it;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    //! \struct AnimObject
    //! \brief  Stores all bf2 objects that can have animation from .inc, .bfmv, and some .con file that defines a skinnedmesh.
    // ------------------------------------------------------------------------------------------------
    struct AnimObject {
        /*enum ObjectType {
        ObjType,
        GroupType
        };*/

        //! Object name
        std::string m_strObjName;
        //! Transformation matrix, stored in OpenGL format
        aiMatrix4x4 m_Transformation;
        //! All sub-objects referenced by this object
        std::vector<Object*> m_SubObjects;
        /// Assigned geometry(mesh)
        std::vector<unsigned int> m_Geometry;

        //! \brief  Default constructor
        AnimObject()
            : m_strObjName("") {
            // empty
        }

        //! \brief  Destructor
        ~AnimObject() {
            for (std::vector<Object*>::iterator it = m_SubObjects.begin(); it != m_SubObjects.end(); ++it) {
                delete *it;
            }
        }
    };
}

// ------------------------------------------------------------------------------------------------
/// \class  BF2Importer
/// \brief  Imports a bf2(Refractor 2 Engine) model/collision/skeleton/animation file
// ------------------------------------------------------------------------------------------------
class BF2Importer : public BaseImporter {
public:
    /// \brief  Default constructor
    BF2Importer();

    /// \brief  Destructor
    ~BF2Importer();

public:
    /// \brief  Returns whether the class can handle the format of the given file.
    /// \remark See BaseImporter::CanRead() for details.
    bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const;

private:
    //! \brief  Appends the supported extension.
    const aiImporterDesc* GetInfo () const;

    //! \brief  File import implementation.
    void InternReadFile(const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);

    //TODO: functions below should be moved into bf2mesh struct
    /*//! \brief  Create the data from imported content.
    void CreateDataFromImport(const BF2File::bf2mesh* pModel, aiScene* pScene);

    //! \brief  Creates all nodes stored in imported content.
    aiNode *createNodes(const BF2File::bf2mesh* pModel, const BF2File::Object* pData,
        aiNode *pParent, aiScene* pScene, std::vector<aiMesh*> &MeshArray);

    //! \brief  Creates topology data like faces and meshes for the geometry.
    aiMesh *createTopology( const BF2File::bf2mesh* pModel, const BF2File::Object* pData,
        unsigned int uiMeshIndex );

    //! \brief  Creates vertices from model.
    void createVertexArray(const BF2File::bf2mesh* pModel, const BF2File::Object* pCurrentObject,
        unsigned int uiMeshIndex, aiMesh* pMesh, unsigned int numIndices );

    //! \brief  Object counter helper method.
    void countObjects(const std::vector<BF2File::Object*> &rObjects, int &iNumMeshes);

    //! \brief  Material creation.
    void createMaterials(const BF2File::bf2mesh* pModel, aiScene* pScene);

    /// @brief  Adds special property for the used texture mapping mode of the model.
    void addTextureMappingModeProperty(aiMaterial* mat, aiTextureType type, int clampMode = 1, int index = 0);

    //! \brief  Appends a child node to a parent node and updates the data structures.
    void appendChildToParentNode(aiNode *pParent, aiNode *pChild);*/

private:
    //! Pointer to root object instance
    BF2File::Object *m_pRootObject;
    //! Absolute pathname of model in file system
    std::string m_strAbsPath;
};

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif
