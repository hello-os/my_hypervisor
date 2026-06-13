#ifndef DT_PARSER_H
#define DT_PARSER_H

#define FDT_MAGIC 0xd00dfeed

struct fdt_header {
    unsigned int magic;
    unsigned int totalsize;
    unsigned int off_dt_struct;
    unsigned int off_dt_strings;
    unsigned int off_mem_rsvmap;
    unsigned int version;
    unsigned int last_comp_version;
    unsigned int boot_cpuid_phys;
    unsigned int size_dt_strings;
    unsigned int size_dt_struct;
};

void dt_init(void *dtb_ptr);
const char *dt_get_node_prop(const char *node_path, const char *prop_name);

#endif
