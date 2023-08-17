#include "assert.h"
#include "heap.h"
#include "queue.h"
#include "uv.h"
#include "uv_os.h"

#define tracef(...) Tracef(uv->tracer, __VA_ARGS__)

/* Metadata about an open segment not used anymore and that should be closed or
 * remove (if not written at all). */
struct uvDyingSegment
{
    struct uv *uv;
    uvCounter counter;      /* Segment counter */
    size_t used;            /* Number of used bytes */
    raft_index first_index; /* Index of first entry */
    raft_index last_index;  /* Index of last entry */
    int status;             /* Status code of blocking syscalls */
    queue queue;            /* Link to finalize queue */
};

double ms_elapsed(struct timespec* start);
struct timespec finalize_start, finalize_work_cb_end;


/* Run all blocking syscalls involved in closing a used open segment.
 *
 * An open segment is closed by truncating its length to the number of bytes
 * that were actually written into it and then renaming it. */
static void uvFinalizeWorkCb(uv_work_t *work)
{
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);

    struct uvDyingSegment *segment = work->data;
    struct uv *uv = segment->uv;
    char filename1[UV__FILENAME_LEN];
    char filename2[UV__FILENAME_LEN];
    char errmsg[RAFT_ERRMSG_BUF_SIZE];
    int rv;

    sprintf(filename1, UV__OPEN_TEMPLATE, segment->counter);
    sprintf(filename2, UV__CLOSED_TEMPLATE, segment->first_index,
            segment->last_index);

    tracef("finalize %s into %s", filename1, filename2);

    /* If the segment hasn't actually been used (because the writer has been
     * closed or aborted before making any write), just remove it. */
    if (segment->used == 0) {
        rv = UvFsRemoveFile(uv->dir, filename1, errmsg);
        if (rv != 0) {
            goto err;
        }
        goto sync;
    }

    /* Truncate and rename the segment.*/
    rv = UvFsTruncateAndRenameFile(uv->dir, segment->used, filename1, filename2,
                                   errmsg);
    if (rv != 0) {
        goto err;
    }

sync:
    rv = UvFsSyncDir(uv->dir, errmsg);
    if (rv != 0) {
        goto err;
    }

    segment->status = 0;

    fprintf(stderr,"\033[102m\033[30m%s took %fms \033[0m\n",
            __FUNCTION__, ms_elapsed(&start));
    clock_gettime(CLOCK_REALTIME, &finalize_work_cb_end);
    return;

err:
    tracef("truncate segment %s: %s", filename1, errmsg);
    assert(rv != 0);
    segment->status = rv;
    clock_gettime(CLOCK_REALTIME, &finalize_work_cb_end);
}



static int uvFinalizeStart(struct uvDyingSegment *segment);
static void uvFinalizeAfterWorkCb(uv_work_t *work, int status)
{
    fprintf(stderr,"\033[42;1mfinalizing segment took %f ms to execute (%fms to call %s)\033[0m\n",
            ms_elapsed(&finalize_start), ms_elapsed(&finalize_work_cb_end), __FUNCTION__);
    struct uvDyingSegment *segment = work->data;
    struct uv *uv = segment->uv;
    tracef("uv finalize after work segment %p cb status:%d", (void *)segment,
           status);
    queue *head;
    int rv;

    assert(status == 0); /* We don't cancel worker requests */
    uv->finalize_work.data = NULL;
    if (segment->status != 0) {
        uv->errored = true;
    }
    RaftHeapFree(segment);

    /* If we have no more dismissed segments to close, check if there's a
     * barrier to unblock or if we are done closing. */
    if (QUEUE_IS_EMPTY(&uv->finalize_reqs)) {
        tracef("unblock barrier or close");
        if (uv->barrier != NULL && UvBarrierReady(uv)) {
            printf(stderr, "\033[45;1m%s: called UvBarrierMaybeTrigger\033[0m\n", __FUNCTION__);
            UvBarrierMaybeTrigger(uv->barrier);
        }
        uvMaybeFireCloseCb(uv);
        return;
    }

    /* Grab a new dismissed segment to close. */
    head = QUEUE_HEAD(&uv->finalize_reqs);
    segment = QUEUE_DATA(head, struct uvDyingSegment, queue);
    QUEUE_REMOVE(&segment->queue);

    rv = uvFinalizeStart(segment);
    if (rv != 0) {
        RaftHeapFree(segment);
        uv->errored = true;
    }
}

/* Start finalizing an open segment. */
static int uvFinalizeStart(struct uvDyingSegment *segment)
{
    struct uv *uv = segment->uv;
    int rv;

    assert(uv->finalize_work.data == NULL);
    assert(segment->counter > 0);

    uv->finalize_work.data = segment;

    clock_gettime(CLOCK_REALTIME, &finalize_start);

//    rv = uv_queue_work(uv->loop, &uv->finalize_work, uvFinalizeWorkCb,
//                       uvFinalizeAfterWorkCb);

// Avoiding uv_queue_work seems to fix the latency problem, although it's not ideal to do it synchronously. Is this also a problem elsewhere?
// A better way to fix this could be a reimplementation of uv_queue_work that calls an uv_async_t object to wake up the loop?
    uvFinalizeWorkCb(&uv->finalize_work);
    uvFinalizeAfterWorkCb(&uv->finalize_work, 0);
    rv = 0;

    if (rv != 0) {
        ErrMsgPrintf(uv->io->errmsg, "start to truncate segment file %llu: %s",
                     segment->counter, uv_strerror(rv));
        return RAFT_IOERR;
    }

    return 0;
}

int UvFinalize(struct uv *uv,
               unsigned long long counter,
               size_t used,
               raft_index first_index,
               raft_index last_index)
{
    struct uvDyingSegment *segment;
    int rv;

    if (used > 0) {
        assert(first_index > 0);
        assert(last_index >= first_index);
    }

    segment = RaftHeapMalloc(sizeof *segment);
    if (segment == NULL) {
        return RAFT_NOMEM;
    }

    segment->uv = uv;
    segment->counter = counter;
    segment->used = used;
    segment->first_index = first_index;
    segment->last_index = last_index;

    /* If we're already processing a segment, let's put the request in the queue
     * and wait. */
    if (uv->finalize_work.data != NULL) {
        QUEUE_PUSH(&uv->finalize_reqs, &segment->queue);
        return 0;
    }

    rv = uvFinalizeStart(segment);
    if (rv != 0) {
        RaftHeapFree(segment);
        return rv;
    }

    return 0;
}

#undef tracef
