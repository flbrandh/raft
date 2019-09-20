#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "../lib/dir.h"
#include "../lib/runner.h"

#include "../../src/uv_error.h"
#include "../../src/uv_os.h"

/******************************************************************************
 *
 * Fixture
 *
 *****************************************************************************/

struct fixture
{
    FIXTURE_DIR;
    uvDir tmpdir; /* Path to a temp directory, defaults to f->dir */
    char errmsg[2048];
};

static void *setup(const MunitParameter params[], void *user_data)
{
    struct fixture *f = munit_malloc(sizeof *f);
    (void)user_data;
    SETUP_DIR;
    strcpy(f->tmpdir, f->dir);
    return f;
}

static void tear_down(void *data)
{
    struct fixture *f = data;
    TEAR_DOWN_DIR;
    free(f);
}

/******************************************************************************
 *
 * Helper macros
 *
 *****************************************************************************/

/* Invoke uvEnsureDir passing it the fixture's tmpdir. */
#define ENSURE_DIR_RV uvEnsureDir(f->tmpdir, f->errmsg)
#define ENSURE_DIR munit_assert_int(ENSURE_DIR_RV, ==, 0)
#define ENSURE_DIR_ERROR(RV) munit_assert_int(ENSURE_DIR_RV, ==, RV)

/* Invoke uvSyncDir passing it the fixture's tmpdir. */
#define SYNC_DIR_RV uvSyncDir(f->tmpdir, f->errmsg)
#define SYNC_DIR munit_assert_int(SYNC_DIR_RV, ==, 0)
#define SYNC_DIR_ERROR(RV) munit_assert_int(SYNC_DIR_RV, ==, RV)

/* Open a file the fixture's tmpdir. */
#define OPEN_FILE_RV(...) uvOpenFile(f->tmpdir, __VA_ARGS__, f->errmsg)
#define OPEN_FILE_ERROR(RV, ...) \
    munit_assert_int(OPEN_FILE_RV(__VA_ARGS__), ==, RV)

/* Invoke uvProbeIoCapabilities against the fixture's tmpdir. */
#define PROBE_IO_CAPABILITIES_RV(...) \
    uvProbeIoCapabilities(f->dir, __VA_ARGS__, f->errmsg)
#define PROBE_IO_CAPABILITIES(DIRECT_IO, ASYNC_IO) \
    munit_assert_int(PROBE_IO_CAPABILITIES_RV(DIRECT_IO, ASYNC_IO), ==, 0)
#define PROBE_IO_CAPABILITIES_ERROR(RV)                            \
    {                                                              \
        size_t direct_io;                                          \
        bool async_io;                                             \
        int rv_ = PROBE_IO_CAPABILITIES_RV(&direct_io, &async_io); \
        munit_assert_int(rv_, ==, RV);                             \
    }

/******************************************************************************
 *
 * Assertions
 *
 *****************************************************************************/

/* Assert that the fixture's errmsg string matches the given value. */
#define ASSERT_ERRMSG(MSG) munit_assert_string_equal(f->errmsg, MSG)

/******************************************************************************
 *
 * uvJoin
 *
 *****************************************************************************/

SUITE(uvJoin)

/* Join a directory path and a filename into a full path. */
TEST(uvJoin, path, NULL, NULL, 0, NULL)
{
    const uvDir dir = "/foo";
    const uvFilename filename = "bar";
    uvPath path;
    (void)data;
    (void)params;
    uvJoin(dir, filename, path);
    munit_assert_string_equal(path, "/foo/bar");
    return MUNIT_OK;
}

/******************************************************************************
 *
 * uvEnsureDir
 *
 *****************************************************************************/

SUITE(uvEnsureDir)

/* If the directory doesn't exist, it is created. */
TEST(uvEnsureDir, does_not_exists, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    sprintf(f->tmpdir, "%s/sub", f->dir);
    ENSURE_DIR;
    munit_assert_true(test_dir_exists(f->tmpdir));
    return MUNIT_OK;
}

/* If the directory exists, nothing is needed. */
TEST(uvEnsureDir, exists, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    ENSURE_DIR;
    return MUNIT_OK;
}

/* If the directory can't be created, an error is returned. */
TEST(uvEnsureDir, mkdir_error, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    strcpy(f->tmpdir, "/foobarbazegg");
    ENSURE_DIR_ERROR(UV__ERROR);
    ASSERT_ERRMSG("mkdir: Permission denied");
    return MUNIT_OK;
}

/* If the directory can't be probed for existence, an error is returned. */
TEST(uvEnsureDir, stat_error, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    strcpy(f->tmpdir, "/proc/1/root");
    ENSURE_DIR_ERROR(UV__ERROR);
    ASSERT_ERRMSG("stat: Permission denied");
    return MUNIT_OK;
}

/* If the given path is not a directory, an error is returned. */
TEST(uvEnsureDir, not_a_dir, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    strcpy(f->tmpdir, "/dev/null");
    ENSURE_DIR_ERROR(UV__ERROR);
    ASSERT_ERRMSG("Not a directory");
    return MUNIT_OK;
}

/******************************************************************************
 *
 * uvSyncDir
 *
 *****************************************************************************/

SUITE(uvSyncDir)

/* If the directory doesn't exist, an error is returned. */
TEST(uvSyncDir, no_exists, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    strcpy(f->tmpdir, "/foobarbazegg");
    SYNC_DIR_ERROR(UV__ERROR);
    ASSERT_ERRMSG("open: No such file or directory");
    return MUNIT_OK;
}

/******************************************************************************
 *
 * uvOpenFile
 *
 *****************************************************************************/

SUITE(uvOpenFile)

/* If the directory doesn't exist, an error is returned. */
TEST(uvOpenFile, no_exists, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    int fd;
    (void)params;
    OPEN_FILE_ERROR(UV__NOENT, "foo", O_RDONLY, &fd);
    ASSERT_ERRMSG("open: No such file or directory");
    return MUNIT_OK;
}

/******************************************************************************
 *
 * uvProbeIoCapabilities
 *
 *****************************************************************************/

SUITE(uvProbeIoCapabilities)

TEST(uvProbeIoCapabilities, tmpfs, setup, tear_down, 0, dir_tmpfs_params)
{
    struct fixture *f = data;
    size_t direct_io;
    bool async_io;
    (void)params;
    PROBE_IO_CAPABILITIES(&direct_io, &async_io);
    munit_assert_false(direct_io);
    munit_assert_false(async_io);
    return MUNIT_OK;
}

#if defined(RAFT_HAVE_ZFS_WITH_DIRECT_IO)

/* ZFS 0.8 reports that it supports direct I/O, but does not support fully
 * asynchronous kernel AIO. */
TEST(uvProbeIoCapabilities, zfs_direct_io, setup, tear_down, 0, dir_zfs_params)
{
    struct fixture *f = data;
    size_t direct_io;
    bool async_io;
    (void)params;
    PROBE_IO_CAPABILITIES(&direct_io, &async_io);
    munit_assert_true(direct_io);
    munit_assert_false(async_io);
    return MUNIT_OK;
}

#elif defined(RAFT_HAVE_ZFS)

TEST_CASE(uvProbeIoCapabilities, zfs, setup, tear_down, 0, dir_zfs_params)
{
    struct fixture *f = data;
    size_t direct_io;
    bool async_io;
    (void)params;
    PROBE_IO_CAPABILITIES(&direct_io, &async_io);
    munit_assert_false(direct_io);
    munit_assert_false(async_io);
    return MUNIT_OK;
}

#endif /* RAFT_HAVE_ZFS_GE_0_8 */

/* If the given path is not executable, the block size of the underlying file
 * system can't be determined and an error is returned. */
TEST(uvProbeIoCapabilities, no_access, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    test_dir_unexecutable(f->dir);
    PROBE_IO_CAPABILITIES_ERROR(UV__ERROR);
    ASSERT_ERRMSG("mkstemp: Permission denied");
    return MUNIT_OK;
}

/* No space is left on the target device. */
TEST(uvProbeIoCapabilities, no_space, setup, tear_down, 0, NULL)
{
    struct fixture *f = data;
    (void)params;
    test_dir_fill(f->dir, 0);
    PROBE_IO_CAPABILITIES_ERROR(UV__ERROR);
    ASSERT_ERRMSG("posix_fallocate: No space left on device");
    return MUNIT_OK;
}

#if defined(RAFT_HAVE_BTRFS) && defined(RWF_NOWAIT)

/* The uvIoSetup() call fails with EAGAIN. */
TEST(uvProbeIoCapabilities, no_resources, setup, tear_down, 0, dir_btrfs_params)
{
    struct fixture *f = data;
    aio_context_t ctx = 0;
    (void)params;
    test_aio_fill(&ctx, 0);
    PROBE_IO_CAPABILITIES_ERROR(UV__ERROR);
    ASSERT_ERRMSG("io_setup: Resource temporarily unavailable");
    test_aio_destroy(ctx);
    return MUNIT_OK;
}

#endif /* RAFT_HAVE_BTRFS && RWF_NOWAIT */
