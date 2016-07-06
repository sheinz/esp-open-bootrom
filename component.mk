
INC_DIRS += $(esp-open-bootrom_ROOT)

esp-open-bootrom_SRC_DIR = $(esp-open-bootrom_ROOT)

$(eval $(call component_compile_rules,esp-open-bootrom))
