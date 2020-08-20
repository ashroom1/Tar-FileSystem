#define FUSE_USE_VERSION 31
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <tar.h>
#include <dirent.h>
#include <fuse.h>

#define RECORD_LENGTH 512
#define open_file_limit 1000

struct open_files
{
    int file_d;
    char *file_name;        //lseek(file_d, 0, SEEK_CUR)
    off_t file_size;
};

char block2[513];
char block[513];
int fd;
char *tar_file;
struct stat stat_g;
size_t open_directories_count = 0;

struct open_files openFiles[open_file_limit];    //Initialize array for opendir calls

_Bool read_block(int fd);

int count_slashes(const char *path);

struct __attribute__((packed)) posix_header
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[167];             /* 345 */
    /* 500 */
};

void* tar_init(struct fuse_conn_info *conn,
               struct fuse_config *cfg)
{
    for (size_t i = 0; i < 10; ++i)
        openFiles[i].file_name = NULL;
    return NULL;
}

off_t tell(int file)
{
    return lseek(file, 0, SEEK_CUR);
}

off_t find_file(const char *path)
{
    FILE *seeking = fopen("/u1/h3/amurali1/seeking", "a+");

    fprintf(seeking, "Seek return value: %ld \n", lseek(fd, 0, SEEK_SET));
    fclose(seeking);
    while (read_block(fd))
    {
        struct posix_header *header = (struct posix_header *) block;
        printf("Read block : %s\n", header->name);
        printf("Path : %s\n", path);
        if (!strcmp(header->name, path))
            return tell(fd);  //Beginning of the block
    }
    return 0;
}

int tar_getattr(const char *path, struct stat * stbuf, struct fuse_file_info *fi)
{

    FILE *attr = fopen("/u1/h3/amurali1/Getattr", "a+");
    fprintf(attr, "---------------------------\n");

    if (!strcmp(path, "/")) {

        stbuf->st_mode = stat_g.st_mode;
        stbuf->st_gid = stat_g.st_gid;
        stbuf->st_uid = stat_g.st_uid;
        stbuf->st_nlink = stat_g.st_nlink;
        stbuf->st_blksize = stat_g.st_blksize;
        stbuf->st_blocks = stat_g.st_blocks;
        stbuf->st_size = stat_g.st_size;
        stbuf->st_mtim = stat_g.st_mtim;

        return 0;
    }
    else
        path += 1;


    fprintf(attr, "Path : %s\n", path);
    fflush(attr);

    if (!find_file(path)) {
        char * path1 = strndup(path, strlen(path) + 2);
        path1[strlen(path)] = '/';
        path1[strlen(path) + 1] = '\0';
        fprintf(attr, "Position of file : %ld\n",find_file(path1));
        fflush(attr);
        }

    struct posix_header *header = (struct posix_header *) block;
    fprintf(attr, "Name from header %s\n", header->name);
    stbuf->st_mode = (mode_t) strtol(header->mode, NULL, 8);

    switch(header->typeflag)
    {
        case '0':
        case '\0': {
            stbuf->st_mode = stbuf->st_mode | 0100000;
            break;
        }
        case '1': {
            stbuf->st_mode = stbuf->st_mode | 0120000;
            break;
        }
        case '2': {
            //Reserved
            break;
        }
        case '3': {
            stbuf->st_mode = stbuf->st_mode | 0020000;
            break;
        }
        case '4': {
            stbuf->st_mode = stbuf->st_mode | 0060000;
            break;
        }
        case '5': {
            stbuf->st_mode = stbuf->st_mode | 0040000;
            break;
        }
        case '6': {
            stbuf->st_mode = stbuf->st_mode | 0010000;
            break;
        }
        case '7': {
            //Reserved
            break;
        }
    }

    stbuf->st_blksize = 512;

    stbuf->st_ino = 0;

    stbuf->st_uid = atoi(header->uid);

    stbuf->st_gid = atoi(header->gid);

    stbuf->st_mtim.tv_sec = strtol(header->mtime, NULL, 8);

    stbuf->st_size = strtol(header->size, NULL, 8);

    stbuf->st_blocks = ceil(stbuf->st_size/ 512);

    stbuf->st_nlink = 1;

    stbuf->st_uid = getuid();

    stbuf->st_gid = getgid();

    fprintf(attr, "Mode : %o\n", stbuf->st_mode);
    fprintf(attr, "Ended!!\n");
    fclose(attr);

    return 0;
}

int tar_read_dir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi,
                 enum fuse_readdir_flags flags)
{

    FILE *rdir = fopen("/u1/h3/amurali1/readdir", "a+");
    fprintf(rdir, "___________________________");



    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    path +=1;


    int num_slashes_input = count_slashes(path);
    fprintf(rdir, "Num slashes path : %d\n", num_slashes_input);
    fprintf(rdir, "Path : %s\n", path);

    lseek(fd, 0, SEEK_SET);
    for (;;) {
        if(read_block(fd))
        {
            struct posix_header * header = (struct posix_header *) block;
            if(header->name[strlen(header->name) - 1] == '/')
                header->name[strlen(header->name) - 1] = '\0';

            if (header->typeflag == REGTYPE || header->typeflag == AREGTYPE)
                lseek(fd, (off_t ) (ceil((float) strtol(header->size, NULL, 8) / 512) * 512), SEEK_CUR);

            int num_slashes_header = count_slashes(header->name);
            fprintf(rdir, "Num slashes header : %d\n", num_slashes_header);

            _Bool slash_equal = 0;

            if (strlen(path) <= 0) {
                if (num_slashes_header == num_slashes_input)
                    slash_equal = 1;
            }
            else if(num_slashes_input + 1 == num_slashes_header)
                slash_equal = 1;

            if (slash_equal && !strcmp(path, strndup(header->name, strlen(path))))
            {
                struct stat st;
                memset(&st, 0, sizeof(st));
                st.st_mode = (mode_t) strtol(header->mode, NULL, 8);

                switch(header->typeflag)
                {
                    case '0':
                    case '\0': {
                        st.st_mode = st.st_mode | 0100000;
                        break;
                    }
                    case '1': {
                        st.st_mode = st.st_mode | 0120000;
                        break;
                    }
                    case '2': {
                        //Reserved
                        break;
                    }
                    case '3': {
                        st.st_mode = st.st_mode | 0020000;
                        break;
                    }
                    case '4': {
                        st.st_mode = st.st_mode | 0060000;
                        break;
                    }
                    case '5': {
                        st.st_mode = st.st_mode | 0040000;
                        break;
                    }
                    case '6': {
                        st.st_mode = st.st_mode | 0010000;
                        break;
                    }
                    case '7': {
                        //Reserved
                        break;
                    }
                }
                st.st_ino = 0;

                char sub_string[100];
                memset(sub_string, 0, 100);
                strcpy(sub_string, header->name + strlen(path));
                fprintf(rdir, "Writing to buf : %s\n", sub_string);

                char * to_fill;
                if (sub_string[0] == '/')
                    to_fill = strdup(sub_string + 1);
                else
                    to_fill = strdup(sub_string);

                if(filler(buf, to_fill, &st, 0, 0))
                    break;
            }
        }
        else
            break;
    }
    fclose(rdir);
    return 0;
}

int count_slashes(const char *path) {
    int num = 0;
    for (size_t i = 0; i < strlen(path); ++i)
        if (path[i] == '/')
            num++;
    return num;
}

_Bool read_block(int fd)
{
    read(fd, block, RECORD_LENGTH);
    size_t j;
    for (j = 0; j < RECORD_LENGTH; ++j)
        if (block[j])
            break;
    if (j == RECORD_LENGTH) {
        read(fd, block2, RECORD_LENGTH);
        for (j = 0; j < RECORD_LENGTH; ++j)
            if (block2[j])
                break;
        if (j == RECORD_LENGTH)
            return 0;
    }
    return 1;
}

static int tar_open(const char *path, struct fuse_file_info *fi)
{

    FILE *open_tar = fopen("/u1/h3/amurali1/tar_open", "a+");
    fprintf(open_tar, "Tar open called with %s\n", path);
    fflush(open_tar);
    fclose(open_tar);
    path += 1;
    int file_d = (int) find_file(path);  //Was - 512

    if (!file_d)
        return -1;

    for (int i = 0; i < open_file_limit; ++i)
        if (!openFiles[i].file_name){
            openFiles[i].file_d = file_d;
            struct posix_header *header = (struct posix_header *) block;
            openFiles[i].file_name = strdup(header->name);
            openFiles[i].file_size = strtol(header->size, NULL, 8);
            fi->fh = file_d;
            return 0;
        }

    return -1;
}

static int tar_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{

    FILE * read_file = fopen("/u1/h3/amurali1/tar_read", "a+");
    int bytes_read;

    if(fi == NULL)
        return -1;

    else {
        for (int i = 0; i < open_file_limit; ++i)
            if (openFiles[i].file_d == fi->fh)
            {
                fprintf(read_file, "File name : %s   size requested : %ld   offset : %ld\n", openFiles[i].file_name, size, offset);

                lseek(fd, fi->fh + offset, SEEK_SET);

                if (offset + size > openFiles[i].file_size) {
                    fprintf(read_file, "Here\n");
                    bytes_read = read(fd, buf, openFiles[i].file_size - offset);
                    fclose(read_file);
                    return bytes_read;
                }
                else {
                    fprintf(read_file, "Here2\n");
                    bytes_read = read(fd, buf, size);
                    fclose(read_file);
                    return bytes_read;
                }
            }
    }
    return -1;
}

static int tar_release(const char *path, struct fuse_file_info *fi)
{
    if (!fi)
        return -1;
    for (int i = 0; i < open_file_limit; ++i)
        if (openFiles[i].file_d == fi->fh)
            openFiles[i].file_name = NULL;
    return 0;
}
/*
static off_t tar_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi)
{
    FILE *file_lseek = fopen("/u1/h3/amurali1/tar_lseek", "a+");
    fprintf(file_lseek, "Test\n\n");
    fflush(file_lseek);
    fclose(file_lseek);
    if (fi == NULL)
        return -1;

    for (int i = 0; i < open_file_limit; ++i) {
        if (openFiles[i].file_d == fi->fh) {
            if (whence == SEEK_CUR)
                openFiles[i].current_offset += off;
            else if (whence == SEEK_END)
                openFiles[i].current_offset = fi->fh + openFiles[i].file_size + off;
            else if (whence == SEEK_SET)
                openFiles[i].current_offset = fi->fh + off;
            return openFiles[i].current_offset;
        }
    }

    return -1;
}

*/

struct fuse_operations tar_oper = {
        .init       = tar_init,
        .getattr    = tar_getattr,
        .readdir    = tar_read_dir,
        .open       = tar_open,
        .read       = tar_read,
        .release    = tar_release,
        //.lseek		= tar_lseek,
};



int main(int argc, char *argv[])
{
    umask(0);
    tar_file = strdup(argv[2]);
    fd = open(tar_file, O_RDWR);


    for (int i = 0; i < open_file_limit; ++i)
        openFiles[i].file_name = NULL;

    fstat(fd, &stat_g);
    stat_g.st_mode = (stat_g.st_mode & (~S_IFREG)) | S_IFDIR;

    argv[2] = NULL;
    return fuse_main(argc - 1, argv, &tar_oper, NULL);
}

