ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS += $(CODE_COVERAGE_CFLAGS)

# Not clobbering the base pointer helps bpftrace construct backtraces
AM_CFLAGS += -fno-omit-frame-pointer

include_HEADERS = include/raft.h

raftincludedir = $(includedir)/raft
raftinclude_HEADERS =

lib_LTLIBRARIES = libraft.la
libraft_la_CFLAGS = $(AM_CFLAGS) -fvisibility=hidden
libraft_la_LDFLAGS = -version-info 3:0:0
libraft_la_SOURCES = \
  src/byte.c \
  src/client.c \
  src/compress.c \
  src/configuration.c \
  src/convert.c \
  src/election.c \
  src/entry.c \
  src/err.c \
  src/flags.c \
  src/heap.c \
  src/lifecycle.c \
  src/log.c \
  src/membership.c \
  src/progress.c \
  src/raft.c \
  src/recv.c \
  src/recv_append_entries.c \
  src/recv_append_entries_result.c \
  src/recv_request_vote.c \
  src/recv_request_vote_result.c \
  src/recv_install_snapshot.c \
  src/recv_timeout_now.c \
  src/replication.c \
  src/snapshot.c \
  src/start.c \
  src/state.c \
  src/syscall.c \
  src/tick.c \
  src/tracing.c

bin_PROGRAMS =

check_PROGRAMS = \
  test/unit/core
TESTS = $(check_PROGRAMS)

check_LTLIBRARIES = libtest.la

libtest_la_CFLAGS = $(AM_CFLAGS) -DMUNIT_TEST_NAME_LEN=60 -Wno-unused-result -Wno-conversion
libtest_la_SOURCES = \
  test/lib/addrinfo.c \
  test/lib/fault.c \
  test/lib/fsm.c \
  test/lib/heap.c \
  test/lib/munit.c \
  test/lib/tracer.c \
  test/lib/tcp.c

test_unit_core_SOURCES = \
  src/byte.c \
  src/compress.c \
  src/configuration.c \
  src/err.c \
  src/flags.c \
  src/heap.c \
  src/log.c \
  test/unit/main_core.c \
  test/unit/test_byte.c \
  test/unit/test_compress.c \
  test/unit/test_configuration.c \
  test/unit/test_err.c \
  test/unit/test_flags.c \
  test/unit/test_log.c \
  test/unit/test_queue.c
test_unit_core_CFLAGS = $(AM_CFLAGS) -Wno-conversion
test_unit_core_LDADD = libtest.la

if LZ4_AVAILABLE
test_unit_core_CFLAGS += -DLZ4_AVAILABLE $(LZ4_CFLAGS)
test_unit_core_LDFLAGS = $(LZ4_LIBS)
libraft_la_CFLAGS += -DLZ4_AVAILABLE $(LZ4_CFLAGS)
libraft_la_LDFLAGS += $(LZ4_LIBS)
endif # LZ4_AVAILABLE
if LZ4_ENABLED
test_unit_core_CFLAGS += -DLZ4_ENABLED
libraft_la_CFLAGS += -DLZ4_ENABLED
endif # LZ4_ENABLED

if FIXTURE_ENABLED

libraft_la_SOURCES += src/fixture.c
raftinclude_HEADERS += include/raft/fixture.h

check_PROGRAMS += \
  test/integration/core \
  test/fuzzy/core

libtest_la_SOURCES += \
  test/lib/cluster.c

test_integration_core_SOURCES = \
  test/integration/main_core.c \
  test/integration/test_apply.c \
  test/integration/test_assign.c \
  test/integration/test_barrier.c \
  test/integration/test_bootstrap.c \
  test/integration/test_digest.c \
  test/integration/test_election.c \
  test/integration/test_fixture.c \
  test/integration/test_heap.c \
  test/integration/test_init.c \
  test/integration/test_membership.c \
  test/integration/test_recover.c \
  test/integration/test_replication.c \
  test/integration/test_snapshot.c \
  test/integration/test_start.c \
  test/integration/test_strerror.c \
  test/integration/test_tick.c \
  test/integration/test_transfer.c
test_integration_core_CFLAGS = $(AM_CFLAGS) -Wno-conversion
test_integration_core_LDFLAGS = -no-install
test_integration_core_LDADD = libtest.la libraft.la

test_fuzzy_core_SOURCES = \
  test/fuzzy/main_core.c \
  test/fuzzy/test_election.c \
  test/fuzzy/test_liveness.c \
  test/fuzzy/test_membership.c \
  test/fuzzy/test_replication.c
test_fuzzy_core_CFLAGS = $(AM_CFLAGS) -Wno-conversion
test_fuzzy_core_LDFLAGS = -no-install
test_fuzzy_core_LDADD = libtest.la libraft.la

endif # FIXTURE_ENABLED

if UV_ENABLED

libraft_la_SOURCES += \
  src/uv.c \
  src/uv_append.c \
  src/uv_encoding.c \
  src/uv_finalize.c \
  src/uv_fs.c \
  src/uv_ip.c \
  src/uv_list.c \
  src/uv_metadata.c \
  src/uv_os.c \
  src/uv_prepare.c \
  src/uv_recv.c \
  src/uv_segment.c \
  src/uv_send.c \
  src/uv_snapshot.c \
  src/uv_tcp.c \
  src/uv_tcp_listen.c \
  src/uv_tcp_connect.c \
  src/uv_truncate.c \
  src/uv_work.c \
  src/uv_writer.c
libraft_la_LDFLAGS += $(UV_LIBS)

raftinclude_HEADERS += include/raft/uv.h

check_PROGRAMS += \
  test/unit/uv \
  test/integration/uv

libtest_la_SOURCES += \
  test/lib/aio.c \
  test/lib/dir.c \
  test/lib/tcp.c \
  test/lib/loop.c

test_unit_uv_SOURCES = \
  src/err.c \
  src/heap.c \
  src/syscall.c \
  src/tracing.c \
  src/uv_fs.c \
  src/uv_os.c \
  src/uv_writer.c \
  test/unit/main_uv.c \
  test/unit/test_uv_fs.c \
  test/unit/test_uv_os.c \
  test/unit/test_uv_writer.c
test_unit_uv_CFLAGS = $(AM_CFLAGS) -Wno-conversion
test_unit_uv_LDADD = libtest.la $(UV_LIBS)

# The integration/uv test is not linked to libraft, but built
# directly against the libraft sources in order to test some
# non-visible, non-API functions.
test_integration_uv_SOURCES = \
  ${libraft_la_SOURCES} \
  test/integration/main_uv.c \
  test/integration/test_uv_init.c \
  test/integration/test_uv_append.c \
  test/integration/test_uv_bootstrap.c \
  test/integration/test_uv_load.c \
  test/integration/test_uv_recover.c \
  test/integration/test_uv_recv.c \
  test/integration/test_uv_send.c \
  test/integration/test_uv_set_term.c \
  test/integration/test_uv_tcp_connect.c \
  test/integration/test_uv_tcp_listen.c \
  test/integration/test_uv_snapshot_put.c \
  test/integration/test_uv_truncate.c \
  test/integration/test_uv_truncate_snapshot.c \
  test/integration/test_uv_work.c
test_integration_uv_CFLAGS = $(AM_CFLAGS) -Wno-type-limits -Wno-conversion
test_integration_uv_LDFLAGS = -no-install
test_integration_uv_LDADD = libtest.la $(UV_LIBS)

AM_CFLAGS += $(UV_CFLAGS)

if LZ4_AVAILABLE
test_integration_uv_CFLAGS += -DLZ4_AVAILABLE
test_integration_uv_LDFLAGS += $(LZ4_LIBS)
endif # LZ4_AVAILABLE
if LZ4_ENABLED
test_integration_uv_CFLAGS += -DLZ4_ENABLED
endif # LZ4_ENABLED

endif # UV_ENABLED

if BACKTRACE_ENABLED
libraft_la_CFLAGS += -DRAFT_ASSERT_WITH_BACKTRACE
libraft_la_LDFLAGS += -lbacktrace
endif # BACKTRACE_ENABLED

if EXAMPLE_ENABLED

bin_PROGRAMS += \
 example/server \
 example/cluster

example_server_SOURCES = example/server.c
example_server_LDFLAGS = -no-install
example_server_LDADD = libraft.la $(UV_LIBS)

example_cluster_SOURCES = example/cluster.c

endif # EXAMPLE_ENABLED

if BENCHMARK_ENABLED

bin_PROGRAMS += \
 benchmark/os-disk-write

benchmark_os_disk_write_SOURCES = benchmark/os_disk_write.c
benchmark_os_disk_write_LDFLAGS = -luring

endif # BENCHMARK_ENABLED

if DEBUG_ENABLED
  AM_CFLAGS += -Werror -Wall
else
  AM_CFLAGS += -DNDEBUG
endif

if SANITIZE_ENABLED
  AM_CFLAGS += -fsanitize=address
endif

if CODE_COVERAGE_ENABLED

include $(top_srcdir)/aminclude_static.am

CODE_COVERAGE_DIRECTORY=./src
CODE_COVERAGE_OUTPUT_DIRECTORY=coverage
CODE_COVERAGE_OUTPUT_FILE=coverage.info
CODE_COVERAGE_IGNORE_PATTERN="/usr/include/*"
CODE_COVERAGE_BRANCH_COVERAGE=1
CODE_COVERAGE_LCOV_OPTIONS=$(CODE_COVERAGE_LCOV_OPTIONS_DEFAULT) --rc lcov_excl_br_line="assert\("

clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean

endif # CODE_COVERAGE_ENABLED

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@.pc
