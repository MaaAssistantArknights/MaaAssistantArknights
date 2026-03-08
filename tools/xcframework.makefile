ARCH ?= $(shell uname -m)
MAA_DEBUG ?= 1

BUILD_DIR := build

XCFRAMEWORKS := $(BUILD_DIR)/MaaCore.xcframework \
                $(BUILD_DIR)/MaaUtils.xcframework \
                $(BUILD_DIR)/fastdeploy_ppocr.xcframework \
                $(BUILD_DIR)/ONNXRuntime.xcframework \
                $(BUILD_DIR)/OpenCV.xcframework

.PHONY: all clean

all: $(XCFRAMEWORKS) $(BUILD_DIR)/compile_commands.json

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/compile_commands.json: build-$(ARCH)/compile_commands.json | $(BUILD_DIR)
	@cp $< $@

ifeq ($(MAA_DEBUG), 0)
    SRC_DIRS := install-arm64 install-x86_64
else
    SRC_DIRS := install-$(ARCH)
endif

_get_and_check = $(if $(filter-out 1,$(words $(wildcard $(1)/$(2)))),\
	$(error Ambiguous dylibs: $(wildcard $(1)/$(2))),\
	$(wildcard $(1)/$(2)))

get_srcs = $(foreach dir,$(SRC_DIRS),$(call _get_and_check,$(dir),$(1)))

$(BUILD_DIR)/MaaCore.xcframework: $(call get_srcs,libMaaCore.dylib)
$(BUILD_DIR)/MaaUtils.xcframework: $(call get_srcs,libMaaUtils.dylib)
$(BUILD_DIR)/fastdeploy_ppocr.xcframework: $(call get_srcs,libfastdeploy_ppocr.dylib)
$(BUILD_DIR)/ONNXRuntime.xcframework: $(call get_srcs,libonnxruntime*.dylib)
$(BUILD_DIR)/OpenCV.xcframework: $(call get_srcs,libopencv*.dylib)

$(BUILD_DIR)/MaaCore.xcframework: EXTRA_ARGS := -headers include

$(XCFRAMEWORKS): | $(BUILD_DIR)
	@rm -rf $@
	@TMP_LIB="$(BUILD_DIR)/$$(basename "$<")"; \
	if [ "$(MAA_DEBUG)" -eq 0 ]; then \
		lipo -create $^ -output "$$TMP_LIB"; \
	else \
		cp "$<" "$$TMP_LIB"; \
	fi; \
	xcodebuild -create-xcframework -library "$$TMP_LIB" $(EXTRA_ARGS) -output $@; \
	rm -f "$$TMP_LIB"

clean:
	rm -rf $(BUILD_DIR)
