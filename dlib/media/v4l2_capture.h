// Copyright (C) 2023  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.

#ifndef DLIB_V4L2_CAPTURE
#define DLIB_V4L2_CAPTURE

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "../logger.h"

namespace dlib
{
    namespace v4l2_impl
    {
        enum io_method {
            IO_METHOD_READ,
            IO_METHOD_MMAP,
            IO_METHOD_USERPTR,
        };

        struct buffer
        {
            void   *start;
            size_t  length;
        };

        static int read_frame(void)
        {
                struct v4l2_buffer buf;
                unsigned int i;

                switch (io) {
                case IO_METHOD_READ:
                        if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
                                switch (errno) {
                                case EAGAIN:
                                        return 0;

                                case EIO:
                                        /* Could ignore EIO, see spec. */

                                        /* fall through */

                                default:
                                        errno_exit("read");
                                }
                        }

                        process_image(buffers[0].start, buffers[0].length);
                        break;

                case IO_METHOD_MMAP:
                        CLEAR(buf);

                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;

                        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                                switch (errno) {
                                case EAGAIN:
                                        return 0;

                                case EIO:
                                        /* Could ignore EIO, see spec. */

                                        /* fall through */

                                default:
                                        errno_exit("VIDIOC_DQBUF");
                                }
                        }

                        assert(buf.index < n_buffers);

                        process_image(buffers[buf.index].start, buf.bytesused);

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                        break;

                case IO_METHOD_USERPTR:
                        CLEAR(buf);

                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;

                        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                                switch (errno) {
                                case EAGAIN:
                                        return 0;

                                case EIO:
                                        /* Could ignore EIO, see spec. */

                                        /* fall through */

                                default:
                                        errno_exit("VIDIOC_DQBUF");
                                }
                        }

                        for (i = 0; i < n_buffers; ++i)
                                if (buf.m.userptr == (unsigned long)buffers[i].start
                                    && buf.length == buffers[i].length)
                                        break;

                        assert(i < n_buffers);

                        process_image((void *)buf.m.userptr, buf.bytesused);

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                        break;
                }

                return 1;
        }

    }

    class v4l2_capture
    {
    public:
        v4l2_capture(std::size_t device);
        ~v4l2_capture();
    private:
        int fd{-1};
    };

    namespace v4l2_impl
    {
        inline dlib::logger& v4l2_logger()
        {
            static dlib::logger GLOBAL("v4l2");
            return GLOBAL;
        }

        inline int xioctl(int fh, int request, void *arg)
        {
            int r;

            do {
                r = ioctl(fh, request, arg);
            } while (-1 == r && EINTR == errno);

            return r;
        }

        inline int open_device(std::size_t device)
        {
            struct stat st;
            char dev_name[16] = {0};
            snprintf(dev_name, sizeof(dev_name), "/dev/video%zu", device);

            if (-1 == stat(dev_name, &st))
            {
                v4l2_logger() << LERROR << " Cannot identify " << dev_name << " : " << errno << " " << strerror(errno);
                return -1;
            }

            if (!S_ISCHR(st.st_mode)) 
            {
                v4l2_logger() << LERROR << dev_name << " is no device";
                return -1;
            }

            int fd = ::open(dev_name, O_RDWR | O_NONBLOCK, 0);

            if (-1 == fd)
            {
                v4l2_logger() << LERROR << "Cannot open " << dev_name << " : " << errno << " " << strerror(errno);
                return -1;
            }

            return fd;
        }
    }

    inline v4l2_capture::v4l2_capture(std::size_t device)
    {
        using namespace v4l2_impl;
        
        if ((fd = open_device(device) < 0))
            return;
    }
    
    inline v4l2::
}

#endif //DLIB_V4L2_CAPTURE