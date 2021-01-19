#include <zip.h>

#include "archive.h"

class ArchiveZip : public Archive {
    class ArchiveFile: public Archive::ArchiveFile {
    public:
        ArchiveFile(zip_file_t *zf_) : zf(zf_) { }
        virtual ~ArchiveFile() { close(); }
        
        virtual void close();
        virtual int64_t read(void *, uint64_t);
        virtual const char *strerror();
        
    private:
        
        zip_file_t *zf;
    };
    
public:
    ArchiveZip(const std::string &name, filetype_t filetype, where_t where, int flags) : Archive(ARCHIVE_ZIP, name, filetype, where, flags), za(NULL) { }
    ArchiveZip(ArchiveContentsPtr contents) : Archive(contents), za(NULL) { }

    virtual ~ArchiveZip() { close(); }

    virtual bool check();
    virtual bool close_xxx();
    virtual bool commit_xxx();
    virtual void commit_cleanup();
    virtual bool file_add_empty_xxx(const std::string &filename);
    virtual bool file_copy_xxx(std::optional<uint64_t> index, Archive *source_archive, uint64_t source_index, const std::string &filename, uint64_t start, std::optional<uint64_t> length);
    virtual bool file_delete_xxx(uint64_t index);
    virtual ArchiveFilePtr file_open(uint64_t index);
    virtual bool file_rename_xxx(uint64_t index, const std::string &filename);
    virtual void get_last_update();
    virtual bool read_infos_xxx();
    virtual bool rollback_xxx(); /* never called if commit never fails */

private:
    zip_t *za;
    
    bool ensure_zip();
    
};
