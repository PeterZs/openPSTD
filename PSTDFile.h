//
// Created by michiel on 18-7-2015.
//

#ifndef OPENPSTD_PSTDFILE_H
#define OPENPSTD_PSTDFILE_H

#include <string>
#include <unqlite.h>
#include "rapidjson/document.h"
#include <memory>
#include <vector>
#include "GeneralTypes.h"

using PSTDFile_Key_t = std::shared_ptr<std::vector<char> >;

enum PSTD_DOMAIN_SIDE
{
    PSTD_DOMAIN_SIDE_TOP,
    PSTD_DOMAIN_SIDE_BOTTOM,
    PSTD_DOMAIN_SIDE_LEFT,
    PSTD_DOMAIN_SIDE_RIGHT,
};

std::string DomainSideToString(PSTD_DOMAIN_SIDE side);

class PSTDFile
{
private:
    bool changed;
    std::shared_ptr<rapidjson::Document> sceneConf;
    unqlite* backend;

    std::unique_ptr<std::string> GetStringValue(PSTDFile_Key_t key);
    char* GetRawValue(PSTDFile_Key_t key, unqlite_int64* nBytes);

    void SetStringValue(PSTDFile_Key_t key, std::shared_ptr<std::string> value);
    void SetRawValue(PSTDFile_Key_t key, unqlite_int64 nBytes, const char *value);
    void DeleteValue(PSTDFile_Key_t key);

    void initilize();
    unsigned int IncrementFrameCount(unsigned int domain);

    PSTDFile_Key_t CreateKey(unsigned int prefix, std::initializer_list<unsigned int> list);
    std::string KeyToString(PSTDFile_Key_t key);

    template<typename T>
    T GetValue(PSTDFile_Key_t key)
    {
        std::unique_ptr<unqlite_int64> nBytes;
        int rc;

        std::unique_ptr<T> zBuf;//Dynamically allocated buffer

        rc = unqlite_kv_fetch(this->backend, key->data(), key->size(), zBuf.get(), nBytes.get());
        if( rc != UNQLITE_OK )
        {
            //todo throw error exception
        }

        return *zBuf;
    }

    template<typename T>
    void SetValue(PSTDFile_Key_t key, T value)
    {
        int rc;

        rc = unqlite_kv_store(this->backend, key->data(), key->size(), &value, sizeof(T));

        if( rc != UNQLITE_OK )
        {
            //todo throw error exception
        }
    }

public:
    static std::unique_ptr<PSTDFile> Open(const std::string &filename);
    static std::unique_ptr<PSTDFile> New(const std::string &filename);
    void Commit();
    void Close();

    std::shared_ptr<rapidjson::Document> GetSceneConf();
    void SetSceneConf(std::shared_ptr<rapidjson::Document> scene);

    std::shared_ptr<rapidjson::Document> GetPSTDConf();
    void SetPSTDConf(std::shared_ptr<rapidjson::Document> PSTD);

    int GetDomainCount();
    int GetFrameCount(unsigned int domain);

    PSTD_FRAME GetFrame(unsigned int frame, unsigned int domain);
    void SaveNextFrame(unsigned int domain, PSTD_FRAME frame);

    void InitilizeSimulationResults(int domains);
    void DeleteSimulationResults();
};


#endif //OPENPSTD_PSTDFILE_H
