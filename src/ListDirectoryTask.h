
#ifndef LIST_DIRECTORYTASK_H_
#define LIST_DIRECTORYTASK_H_

#include "PosixTask.h"

namespace storagemanager
{

class ListDirectoryTask : public PosixTask
{
    public:
        ListDirectoryTask(int sock, uint length);
        virtual ~ListDirectoryTask();
        
        void run();
    
    private:
        ListDirectoryTask();
        
        void writeString(uint8_t buf, int *offset, int size, const std::string &str)
        struct cmd_overlay {
            uint flen;
            char path[];
        };
};


}
#endif
