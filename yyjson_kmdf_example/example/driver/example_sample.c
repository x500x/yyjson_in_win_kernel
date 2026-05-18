#include "example_sample.h"

#include "kprintf.h"
#include "yyjson.h"
#include "yyjsonk_runtime.h"

#define YYJSONK_EXAMPLE_FILE_PATH "C:\\Windows\\Temp\\yyjson_kmdf_example.json"

static NTSTATUS
YYJSONK_RunParseExample(VOID)
{
    static char sample_json[] =
        "{\"name\":\"yyjson\",\"version\":1,\"enabled\":true,\"message\":\"hello kernel\"}";
    yyjson_read_err err;
    yyjson_doc *doc;
    yyjson_val *root;
    yyjson_val *name;
    yyjson_val *version;
    yyjson_val *enabled;
    yyjson_val *message;
    char *pretty_json;
    NTSTATUS status = STATUS_SUCCESS;

    memset(&err, 0, sizeof(err));
    kprintf("parse example input:\n%s\n", sample_json);

    doc = yyjson_read_opts(sample_json,
                           sizeof(sample_json) - 1,
                           0,
                           NULL,
                           &err);
    if (!doc) {
        kprintf("parse failed code=%u msg=%s pos=%zu\n",
                err.code,
                err.msg ? err.msg : "<none>",
                err.pos);
        return STATUS_UNSUCCESSFUL;
    }

    root = yyjson_doc_get_root(doc);
    if (!root || !yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return STATUS_INVALID_PARAMETER;
    }

    name = yyjson_obj_get(root, "name");
    version = yyjson_obj_get(root, "version");
    enabled = yyjson_obj_get(root, "enabled");
    message = yyjson_obj_get(root, "message");

    kprintf("parsed name=%s version=%lld enabled=%s\n",
            (name && yyjson_is_str(name)) ? yyjson_get_str(name) : "<missing>",
            (version && yyjson_is_num(version)) ? (long long)yyjson_get_sint(version) : -1,
            (enabled && yyjson_is_bool(enabled) && yyjson_get_bool(enabled)) ? "true" : "false");
    kprintf("parsed message=%s\n",
            (message && yyjson_is_str(message)) ? yyjson_get_str(message) : "<missing>");

    pretty_json = yyjson_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
    if (!pretty_json) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        kprintf("parsed document pretty output:\n%s\n", pretty_json);
        free(pretty_json);
    }

    yyjson_doc_free(doc);
    return status;
}

static NTSTATUS
YYJSONK_RunArrayTraversalExample(VOID)
{
    static char sample_json[] =
        "{"
        "\"requests\":["
        "{\"op\":\"parse\",\"status\":0,\"bytes\":128},"
        "{\"op\":\"write\",\"status\":0,\"bytes\":256},"
        "{\"op\":\"file\",\"status\":0,\"bytes\":512}"
        "]"
        "}";
    yyjson_read_err err;
    yyjson_doc *doc;
    yyjson_val *root;
    yyjson_val *requests;
    yyjson_arr_iter iter;
    yyjson_val *entry;
    size_t index = 0;
    size_t success_count = 0;
    unsigned long long total_bytes = 0;

    memset(&err, 0, sizeof(err));
    kprintf("array traversal example input:\n%s\n", sample_json);

    doc = yyjson_read_opts(sample_json,
                           sizeof(sample_json) - 1,
                           0,
                           NULL,
                           &err);
    if (!doc) {
        kprintf("array traversal parse failed code=%u msg=%s pos=%zu\n",
                err.code,
                err.msg ? err.msg : "<none>",
                err.pos);
        return STATUS_UNSUCCESSFUL;
    }

    root = yyjson_doc_get_root(doc);
    requests = root ? yyjson_obj_get(root, "requests") : NULL;
    if (!requests || !yyjson_is_arr(requests)) {
        yyjson_doc_free(doc);
        return STATUS_INVALID_PARAMETER;
    }

    yyjson_arr_iter_init(requests, &iter);
    while ((entry = yyjson_arr_iter_next(&iter)) != NULL) {
        yyjson_val *op = yyjson_obj_get(entry, "op");
        yyjson_val *status_val = yyjson_obj_get(entry, "status");
        yyjson_val *bytes = yyjson_obj_get(entry, "bytes");
        int status_code = yyjson_is_int(status_val) ? yyjson_get_int(status_val) : -1;
        unsigned long long byte_count =
            yyjson_is_uint(bytes) ? (unsigned long long)yyjson_get_uint(bytes) : 0;

        if (status_code == 0) {
            success_count++;
        }
        total_bytes += byte_count;

        kprintf("request[%zu] op=%s status=%d bytes=%llu\n",
                index,
                (op && yyjson_is_str(op)) ? yyjson_get_str(op) : "<missing>",
                status_code,
                byte_count);
        index++;
    }

    kprintf("array traversal summary success=%zu total_bytes=%llu\n",
            success_count,
            total_bytes);
    yyjson_doc_free(doc);
    return STATUS_SUCCESS;
}

static NTSTATUS
YYJSONK_LogGeneratedDocument(VOID)
{
    yyjson_mut_doc *doc;
    yyjson_mut_val *root;
    yyjson_mut_val *items;
    char *json;
    NTSTATUS status = STATUS_SUCCESS;

    doc = yyjson_mut_doc_new(NULL);
    if (!doc) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    root = yyjson_mut_obj(doc);
    items = yyjson_mut_arr(doc);
    if (!root || !items) {
        yyjson_mut_doc_free(doc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_arr_add_str(doc, items, "kmdf");
    yyjson_mut_arr_add_str(doc, items, "yyjson");
    yyjson_mut_arr_add_str(doc, items, "example");
    yyjson_mut_obj_add_str(doc, root, "driver", "example.sys");
    yyjson_mut_obj_add_str(doc, root, "library", "yyjson");
    yyjson_mut_obj_add_bool(doc, root, "unload_supported", true);
    yyjson_mut_obj_add_uint(doc, root, "value", 42);
    yyjson_mut_obj_add_val(doc, root, "tags", items);

    json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
    if (!json) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        kprintf("generated json:\n%s\n", json);
        free(json);
    }

    yyjson_mut_doc_free(doc);
    return status;
}

static NTSTATUS
YYJSONK_RunFileExample(VOID)
{
    yyjson_mut_doc *out_doc;
    yyjson_mut_val *root;
    yyjson_mut_val *events;
    yyjson_write_err write_err;
    yyjson_read_err read_err;
    yyjson_doc *in_doc;
    yyjson_val *in_root;
    yyjson_val *driver;
    yyjson_val *events_read;
    char *pretty_json;
    bool built;

    memset(&write_err, 0, sizeof(write_err));
    memset(&read_err, 0, sizeof(read_err));

    out_doc = yyjson_mut_doc_new(NULL);
    if (!out_doc) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    root = yyjson_mut_obj(out_doc);
    events = yyjson_mut_arr(out_doc);
    if (!root || !events) {
        yyjson_mut_doc_free(out_doc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    yyjson_mut_doc_set_root(out_doc, root);
    built = yyjson_mut_obj_add_str(out_doc, root, "driver", "example.sys") &&
            yyjson_mut_obj_add_str(out_doc, root, "mode", "kernel") &&
            yyjson_mut_obj_add_bool(out_doc, root, "passive_level_only", true) &&
            yyjson_mut_obj_add_uint(out_doc, root, "schema_version", 1) &&
            yyjson_mut_arr_add_str(out_doc, events, "create-json-document") &&
            yyjson_mut_arr_add_str(out_doc, events, "write-json-file") &&
            yyjson_mut_arr_add_str(out_doc, events, "read-json-file") &&
            yyjson_mut_obj_add_val(out_doc, root, "events", events);
    if (!built) {
        yyjson_mut_doc_free(out_doc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!yyjson_mut_write_file(YYJSONK_EXAMPLE_FILE_PATH,
                               out_doc,
                               YYJSON_WRITE_PRETTY_TWO_SPACES,
                               NULL,
                               &write_err)) {
        kprintf("file write failed path=%s code=%u msg=%s\n",
                YYJSONK_EXAMPLE_FILE_PATH,
                write_err.code,
                write_err.msg ? write_err.msg : "<none>");
        yyjson_mut_doc_free(out_doc);
        return STATUS_UNSUCCESSFUL;
    }
    yyjson_mut_doc_free(out_doc);

    in_doc = yyjson_read_file(YYJSONK_EXAMPLE_FILE_PATH, 0, NULL, &read_err);
    if (!in_doc) {
        kprintf("file read failed path=%s code=%u msg=%s pos=%zu\n",
                YYJSONK_EXAMPLE_FILE_PATH,
                read_err.code,
                read_err.msg ? read_err.msg : "<none>",
                read_err.pos);
        return STATUS_UNSUCCESSFUL;
    }

    in_root = yyjson_doc_get_root(in_doc);
    driver = in_root ? yyjson_obj_get(in_root, "driver") : NULL;
    events_read = in_root ? yyjson_obj_get(in_root, "events") : NULL;

    kprintf("file example path=%s driver=%s events=%zu\n",
            YYJSONK_EXAMPLE_FILE_PATH,
            (driver && yyjson_is_str(driver)) ? yyjson_get_str(driver) : "<missing>",
            events_read ? yyjson_arr_size(events_read) : 0);

    pretty_json = yyjson_write(in_doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
    if (pretty_json) {
        kprintf("file example readback:\n%s\n", pretty_json);
        free(pretty_json);
    } else {
        yyjson_doc_free(in_doc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    yyjson_doc_free(in_doc);
    return STATUS_SUCCESS;
}

NTSTATUS
yyjsonk_run_example(VOID)
{
    NTSTATUS status;

    kprintf("yyjson version=0x%08X\n", yyjson_version());

    status = YYJSONK_RunParseExample();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = YYJSONK_RunArrayTraversalExample();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = YYJSONK_LogGeneratedDocument();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = YYJSONK_RunFileExample();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    kprintf("%s", "yyjson example completed successfully\n");
    return STATUS_SUCCESS;
}
