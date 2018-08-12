#ifndef ASSIMP_BUILD_NO_BF2_IMPORTER

#include "BF2Importer.h"
#include "IOStreamBuffer.h"
#include <memory>
#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/ai_assert.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/importerdesc.h>

static const aiImporterDesc desc = {
    "BF2 File Importer",
    "worldlife123",
    "worldlife123",
    "still in development",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "con inc bundledmesh skinnedmesh staticmesh collisionmesh ske baf bfmv"
};

static const std::string simpleExtensions[] = {"bundledmesh", "skinnedmesh", "staticmesh", "collisionmesh", "ske", "baf"};

static const std::string batchExtensions[] = {"con", "inc", "bfmv"};

static const unsigned int MinSize = 20; //bf2header is 20 bytes

namespace Assimp {

using namespace BF2File;

// ------------------------------------------------------------------------------------------------
//  Default constructor
BF2Importer::BF2Importer() :
    m_pRootObject( NULL ),
    m_strAbsPath( "" )
{
    DefaultIOSystem io;
    m_strAbsPath = io.getOsSeparator();
}

// ------------------------------------------------------------------------------------------------
//  Destructor.
BF2Importer::~BF2Importer()
{
    delete m_pRootObject;
    m_pRootObject = NULL;
}

// ------------------------------------------------------------------------------------------------
//  Returns true, if file is an bf2 file.
bool BF2Importer::CanRead( const std::string& pFile, IOSystem*  pIOHandler , bool checkSig ) const
{
    if(!checkSig) //Check File Extension
    {
        const std::string extension = GetExtension(pFile);

        for (auto ext : simpleExtensions) {
            if (!ASSIMP_stricmp(extension, ext))
                return true;
        }
        for (auto ext : batchExtensions) {
            if (!ASSIMP_stricmp(extension, ext))
                return true;
        }

        //not bf2 file extensions
        return false;
    }
    else //Check file Header
    {
        // TODO:It's hard to check by header(both text and binary), so just pass test... 
        // #Assimp::BaseImporter::CheckMagicToken - for binary files. It goes
        // to a particular offset in the file and and compares the next words
        // against a given list of 'magic' tokens.
        return true;
    }
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc* BF2Importer::GetInfo () const
{
    return &desc;
}

// ------------------------------------------------------------------------------------------------
//  bf2 file format import implementation
void BF2Importer::InternReadFile( const std::string &file, aiScene* pScene, IOSystem* pIOHandler) {
    // Get extension to decide how to read
    const std::string extension = GetExtension(file); // note that the extension is lowercase
    bool isBatchFile = false;
    for (auto ext : batchExtensions) {
        if (!ASSIMP_stricmp(extension, ext)) {
            isBatchFile = true;
            break;
        }
    }

    // Read file into memory
    static const std::string mode = "rb";
    std::unique_ptr<IOStream> fileStream( pIOHandler->Open( file, mode));
    if( !fileStream.get() ) {
        throw DeadlyImportError( "Failed to open file " + file + "." );
    }

    // Get the file-size and validate it, throwing an exception when fails
    size_t fileSize = fileStream->FileSize();
    if( fileSize < MinSize ) {
        throw DeadlyImportError( "BF2-file is too small.");
    }

    // Get the model name
    std::string  fileName, folderName, modelName;
    std::string::size_type pos = file.find_last_of( "\\/" );
    if ( pos != std::string::npos ) {
        fileName = file.substr(pos+1, file.size() - pos - 1);
        folderName = file.substr( 0, pos );
        if ( !folderName.empty() ) {
            pIOHandler->PushDirectory( folderName );
        }
    }
    else {
        fileName = file;
    }
    pos = fileName.find_last_of('.');
    if (pos == std::string::npos) modelName = fileName;
    else modelName = fileName.substr(0, pos);

    if (isBatchFile) { //read in text mode
        IOStreamBuffer<char> streamedBuffer;
        streamedBuffer.open(fileStream.get());

        BatchLoader batchLoader(pIOHandler, false);

        // Allocate buffer and read file into it
        //TextFileToBuffer( fileStream.get(),m_Buffer);

        streamedBuffer.close();

        // 1/3rd progress
        m_progress->UpdateFileRead(1, 3);

        // waits until all file is loaded
        batchLoader.LoadAll();
        // BF2FileParser parser( streamedBuffer, modelName, pIOHandler, m_progress, file);
        
    }
    else { //read in binary mode

        if ("collisionmesh" == extension) {//bf2col
            bf2col colObject;
            if (colObject.Load(file.c_str()) != 0) throw DeadlyImportError("BF2: bf2col import error!");
            // 1/3rd progress
            m_progress->UpdateFileRead(1, 3);
            // And create the proper return structures out of it
            colObject.createScene(pScene, modelName);
        }
        else if ("ske" == extension){//bf2ske
            bf2ske skeObject;
            if (skeObject.Load(file.c_str()) != 0) throw DeadlyImportError("BF2: bf2ske import error!");
            // 1/3rd progress
            m_progress->UpdateFileRead(1, 3);
            // And create the proper return structures out of it
            skeObject.createScene(pScene, modelName);
        }
        else if ("baf" == extension){//bf2anim
            bf2baf animObject;
            if (animObject.Load(file.c_str()) != 0) throw DeadlyImportError("BF2: bf2baf import error!");
            // 1/3rd progress
            m_progress->UpdateFileRead(1, 3);
            // And create the proper return structures out of it
            animObject.createScene(pScene, modelName);
        }
        else {//bf2mesh
            bf2mesh meshObject;
            if (meshObject.Load(file.c_str(), extension.c_str())!=0) throw DeadlyImportError("BF2: bf2mesh import error!");
            // 1/3rd progress
            m_progress->UpdateFileRead(1, 3);
            // And create the proper return structures out of it
            meshObject.createScene(pScene,modelName);
        }
    } 

    // 2/3rd progress
    m_progress->UpdateFileRead(2, 3);

    //CreateDataFromImport(parser.GetModel(), pScene);

    // 3/3rd progress
    m_progress->UpdateFileRead(3, 3);

    // Pop directory stack
    if ( pIOHandler->StackSize() > 0 ) {
        pIOHandler->PopDirectory();
    }
}

}   // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
