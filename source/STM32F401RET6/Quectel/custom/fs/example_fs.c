#include "QuectelConfig.h"
#ifdef __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__
#include "bg95_filesystem.h"
#include "at_osal.h"
#include "debug_service.h"
#include "broadcast_service.h"
#include "debug_service.h"
#include "example_fs.h"

// Function to print a list of files with their details
void PrintfsList(File_Moudle_Info *fileList, int fileCount)
{
    for (int i = 0; i < fileCount; i++)
    {
        // Print the filename, size in kilobytes, and date for each file.
        LOG_V("Filename: %s       , B: %ld      \n",
              fileList[i].filename,
              fileList[i].filesize);
    }
}
char data[30 + 1];
int user_fs_test(void *argument)
{
    fs_test_config *config = (fs_test_config *)argument;
    if (config->fs_type == 0)
    {
        File_Moudle_Info fileList[5];
        int fileCount = ql_module_list_get(config->name_pattern, fileList, sizeof(fileList) / sizeof(fileList[0]), 0);
        if (fileCount >= 0)
        {
            PrintfsList(fileList, fileCount);
        }
    }
    else if (config->fs_type == 1) // DEL
    {
        ql_file_del(config->name_pattern);
    }
    else if (config->fs_type == 2) // FREE
    {
        rt_size_t free_size, total_size;
        LOG_V("name_pattern %s.\n", config->name_pattern);
        if (QL_fs_free(config->name_pattern, &free_size, &total_size) == 0)
        {
            LOG_V("Free size: %d, Total size: %d\n", free_size, total_size);
        }
        else
        {
            LOG_V("Failed to get file system info.\n");
        }
    }
    else if (config->fs_type == 3) // open
    {
        u8_t file_handle;
        file_handle = QL_fs_open(config->name_pattern, config->open_mode);
        LOG_V("file_handle %d\n", file_handle);
    }
    else if (config->fs_type == 4) // open
    {

        QL_fs_write(config->file_handle, config->wirte_read_size, config->wirte_buffer);
    }
    else if (config->fs_type == 5) // open
    {
        QL_fs_close(config->file_handle);
    }
    else if (config->fs_type == 6) // open
    {

        u8_t read_len = QL_fs_read(config->file_handle, config->wirte_read_size, data);
        if (read_len > 0)
        {
            LOG_V("Read data (%d bytes): %s\n", read_len, data);
        }
        else
        {
            LOG_V("Failed to read data.\n");
        }
    }
}

#endif /* __QUECTEL_USER_FRIENDLY_PROJECT_FEATURE_SUPPORT_FILESYSTEM_EXAMPLE__ */